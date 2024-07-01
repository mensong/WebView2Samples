// Plugin.System.cpp : 定义 DLL 应用程序的导出函数。
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


//将Ansi字符转换为Unicode字符串
std::wstring AnsiToUnicode(const std::string& multiByteStr)
{
	wchar_t* pWideCharStr; //定义返回的宽字符指针
	int nLenOfWideCharStr; //保存宽字符个数，注意不是字节数
	const char* pMultiByteStr = multiByteStr.c_str();
	//获取宽字符的个数
	nLenOfWideCharStr = MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, -1, NULL, 0);
	//获得宽字符指针
	pWideCharStr = (wchar_t*)(HeapAlloc(GetProcessHeap(), 0, nLenOfWideCharStr * sizeof(wchar_t)));
	MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, -1, pWideCharStr, nLenOfWideCharStr);
	//返回
	std::wstring wideByteRet(pWideCharStr, nLenOfWideCharStr);
	//销毁内存中的字符串
	HeapFree(GetProcessHeap(), 0, pWideCharStr);
	return wideByteRet.c_str();
}

//将Unicode字符转换为Ansi字符串
std::string UnicodeToAnsi(const std::wstring& wideByteStr)
{
	char* pMultiCharStr; //定义返回的多字符指针
	int nLenOfMultiCharStr; //保存多字符个数，注意不是字节数
	const wchar_t* pWideByteStr = wideByteStr.c_str();
	//获取多字符的个数
	nLenOfMultiCharStr = WideCharToMultiByte(CP_ACP, 0, pWideByteStr, -1, NULL, 0, NULL, NULL);
	//获得多字符指针
	pMultiCharStr = (char*)(HeapAlloc(GetProcessHeap(), 0, nLenOfMultiCharStr * sizeof(char)));
	WideCharToMultiByte(CP_ACP, 0, pWideByteStr, -1, pMultiCharStr, nLenOfMultiCharStr, NULL, NULL);
	//返回
	std::string sRet(pMultiCharStr, nLenOfMultiCharStr);
	//销毁内存中的字符串
	HeapFree(GetProcessHeap(), 0, pMultiCharStr);
	return sRet.c_str();
}

std::string UnicodeToUtf8(const std::wstring& wideByteStr)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, wideByteStr.c_str(), -1, NULL, 0, NULL, NULL);
	char* szUtf8 = new char[len + 1];
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wideByteStr.c_str(), -1, szUtf8, len, NULL, NULL);
	std::string s = szUtf8;
	delete[] szUtf8;
	return s.c_str();
}

std::wstring Utf8ToUnicode(const std::string& utf8Str)
{
	//预转换，得到所需空间的大小;
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, utf8Str.c_str(), strlen(utf8Str.c_str()), NULL, 0);
	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	//转换
	::MultiByteToWideChar(CP_UTF8, NULL, utf8Str.c_str(), strlen(utf8Str.c_str()), wszString, wcsLen);
	//最后加上'\0'
	wszString[wcsLen] = '\0';
	std::wstring s(wszString);
	delete[] wszString;
	return s;
}

std::string AnsiToUtf8(const std::string& multiByteStr)
{
	std::wstring ws = AnsiToUnicode(multiByteStr);
	return UnicodeToUtf8(ws);
}

std::string Utf8ToAnsi(const std::string& utf8Str)
{
	std::wstring ws = Utf8ToUnicode(utf8Str);
	return UnicodeToAnsi(ws);
}


//插件入口点
PLUGIN_API int plugin_entry(class AppWindow* appWindow)
{
	return 0;
}

//插件名称
PLUGIN_API const wchar_t* plugin_name()
{
	return L"System";
}

//插件方法列表
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
		WebViewApi::SetResultString(stringResult, sContext.c_str());
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
	std::string utext = UnicodeToUtf8(text);
	bool append = js->Child(L"append")->AsBool();

	std::locale loc = std::locale::global(std::locale(std::locale(), "", LC_CTYPE));

	std::ios_base::openmode m = std::ios_base::out;
	if (append)
		m |= std::ios_base::app;
	std::ofstream fout(file.c_str(), m);
	if (fout.good())
	{
		fout.write(utext.c_str(), utext.size());

		fout.close();

		WebViewApi::SetResultString(stringResult, L"true");
	}
	else
	{
		WebViewApi::SetResultString(stringResult, L"false");
	}

	//std::locale utf8(std::locale("C"), new std::codecvt_utf8<wchar_t, 0x10ffff, std::codecvt_mode(std::generate_header | std::little_endian)>());
	//fout.imbue(utf8);
	
	std::locale::global(loc);
	return S_OK;
}

PLUGIN_API HRESULT pathExist(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult)
{
	if (::PathFileExistsW(stringParamters) == TRUE)
		WebViewApi::SetResultString(stringResult, L"true");
	else
		WebViewApi::SetResultString(stringResult, L"false");
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
	exit(0);
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

	//写长度
	ProceedDataExchange::RESULT res = g_shm.writePackage(&dataSize, sizeof(dataSize), id * 2, TRUE);
	if (res < 0)
	{
		return E_FAIL;
	}

	//写内容
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

	JSONValue* js = JSON::Parse(stringParamters);
	if (js == NULL)
	{
		return E_FAIL;
	}

	long id = -1;

	bool hasDef = false;
	std::wstring defVal;

	if (js->IsInt())
	{
		id = js->AsNumber();
		hasDef = false;
	}
	else
	{
		if (!js->HasChild(L"id"))
			return E_FAIL;
		id = js->Child(L"id")->AsNumber(-1);
		defVal = js->Child(L"default")->AsString();
		hasDef = true;
	}

	if (id < 0)
		return E_FAIL;

	long dataSize = 0;
	ProceedDataExchange::RESULT res = g_shm.readPackage(&dataSize, sizeof(dataSize), id * 2, TRUE);
	if (res < 0)
	{
		if (hasDef)
		{
			WebViewApi::SetResultString(stringResult, defVal.c_str());
			return S_OK;
		}
		return E_FAIL;
	}

	long realDataSize = dataSize * sizeof(wchar_t);
	wchar_t* data = new wchar_t[dataSize + 1];
	res = g_shm.readPackage(data, realDataSize, (id * 2) + 1, TRUE);
	if (res < 0)
	{
		delete[] data;

		if (hasDef)
		{
			WebViewApi::SetResultString(stringResult, defVal.c_str());
			return S_OK;
		}
		return E_FAIL;
	}
	data[dataSize] = '\0';

	WebViewApi::SetResultString(stringResult, data);
	delete[] data;

	return S_OK;
}