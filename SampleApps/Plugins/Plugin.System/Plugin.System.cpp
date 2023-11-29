// Plugin.System.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "pch.h"
#include "WebViewApi.h"
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include "json.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "ProceedDataExchange.h"
ProceedDataExchange g_shm(_T("WebView2Host"), 10240);

//�����ڵ�
PLUGIN_API int plugin_entry(class AppWindow* appWindow)
{
	return 0;
}

//�������
PLUGIN_API const wchar_t* plugin_name()
{
	return L"System";
}

//��������б�
PLUGIN_API const wchar_t* plugin_functions()
{
	return
		L"readText\0"
		L"writeText\0"
		L"pathExist\0"
		L"setIcon\0"
		L"setCaption\0"
		L"killMe\0"
		L"setSharedData\0"
		L"getSharedData\0"
		L"\0"
		;
}

PLUGIN_API HRESULT readText(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	std::wifstream fin(stringParamters);
	if (fin.good())
	{
		std::wstringstream ssContent;
		ssContent << fin.rdbuf();
		std::wstring sContext = ssContent.str();
		fin.close();
		*stringResult = SysAllocString(sContext.c_str());
		return S_OK;
	}

	return E_FAIL;
}

PLUGIN_API HRESULT writeText(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	JSONValue* js = JSON::Parse(stringParamters);
	if (js == NULL)
	{
		return E_FAIL;
	}

	std::wstring file = js->Child(L"file")->AsString();
	if (file.empty())
		return E_FAIL;
	std::wstring text = js->Child(L"text")->AsString();
	bool append = js->Child(L"append")->AsBool();

	std::locale loc = std::locale::global(std::locale(std::locale(), "", LC_CTYPE));

	std::ios_base::openmode m = std::ios_base::out;
	if (append)
		m |= std::ios_base::app;
	std::wofstream fout(file.c_str(), m);
	if (fout.good())
	{
		fout.write(text.c_str(), text.size());

		fout.close();

		*stringResult = SysAllocString(L"true");
	}
	else
	{
		*stringResult = SysAllocString(L"false");
	}

	//std::locale utf8(std::locale("C"), new std::codecvt_utf8<wchar_t, 0x10ffff, std::codecvt_mode(std::generate_header | std::little_endian)>());
	//fout.imbue(utf8);
	
	std::locale::global(loc);
	return S_OK;
}

PLUGIN_API HRESULT pathExist(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	if (::PathFileExistsW(stringParamters) == TRUE)
		*stringResult = SysAllocString(L"true");
	else 
		*stringResult = SysAllocString(L"false");
	return S_OK;
}

PLUGIN_API HRESULT setIcon(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	HICON hicon = (HICON)LoadImageW(
		NULL, //handle of the instance that contains //the image
		stringParamters,//name or identifier of image
		IMAGE_ICON,//type of image-can also be IMAGE_CURSOR or MAGE_ICON
		0, 0,//desired width and height
		LR_LOADFROMFILE);//load flags
	if (hicon == NULL)
		return E_FAIL;
	
	SendMessage(WebViewApi::Ins().GetHWND(appWindow), WM_SETICON, TRUE, (LPARAM)hicon);
	SendMessage(WebViewApi::Ins().GetHWND(appWindow), WM_SETICON, FALSE, (LPARAM)hicon);

	return S_OK;
}

PLUGIN_API HRESULT setCaption(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	::SetWindowText(WebViewApi::Ins().GetHWND(appWindow), stringParamters);
	return S_OK;
}

PLUGIN_API HRESULT killMe(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	WebViewApi::Ins().CloseApp(appWindow);

	return S_OK;
}

PLUGIN_API HRESULT setSharedData(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	if (!g_shm.isValid())
	{
		return E_FAIL;
	}

	JSONValue* js = JSON::Parse(stringParamters);
	if (js == NULL)
	{
		return E_FAIL;
	}

	long id = js->Child(L"id")->AsNumber(-1);
	if (id < 0)
		return E_FAIL;
	std::wstring text = js->Child(L"value")->AsString();
	
	long dataSize = wcslen(text.c_str());

	//д����
	ProceedDataExchange::RESULT res = g_shm.writePackage(&dataSize, sizeof(dataSize), id * 2, TRUE);
	if (res < 0)
	{
		return E_FAIL;
	}

	//д����
	res = g_shm.writePackage(text.c_str(), dataSize * sizeof(wchar_t), (id * 2) + 1, TRUE);
	if (res < 0)
	{
		return E_FAIL;
	}

	return S_OK;
}

PLUGIN_API HRESULT getSharedData(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	if (!g_shm.isValid())
	{
		return E_FAIL;
	}

	long id = _wtoi(stringParamters);
	if (id < 0)
		return E_FAIL;

	long dataSize = 0;
	ProceedDataExchange::RESULT res = g_shm.readPackage(&dataSize, sizeof(dataSize), id * 2, TRUE);
	if (res < 0)
	{
		return E_FAIL;
	}

	long realDataSize = dataSize * sizeof(wchar_t);
	wchar_t* data = new wchar_t[dataSize + 1];
	res = g_shm.readPackage(data, realDataSize, (id * 2) + 1, TRUE);
	if (res < 0)
	{
		delete[] data;
		return E_FAIL;
	}
	data[dataSize] = '\0';

	*stringResult = SysAllocString(data);
	delete[] data;

	return S_OK;
}