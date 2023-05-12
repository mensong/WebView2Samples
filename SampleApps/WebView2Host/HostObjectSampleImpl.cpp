// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "HostObjectSampleImpl.h"
#include <Windows.h>
#include "CheckFailure.h"
#include <fstream>
#include <sstream>
#include <codecvt>

HostObjectSample::HostObjectSample(HostObjectSample::RunCallbackAsync runCallbackAsync, AppWindow* appWindow)
    : m_propertyValue(L"")
    , m_runCallbackAsync(runCallbackAsync)
    , m_appWindow(appWindow)
{
    
}

STDMETHODIMP HostObjectSample::MethodWithParametersAndReturnValue(BSTR stringParameter, INT integerParameter, BSTR* stringResult)
{
    std::wstring result = L"MethodWithParametersAndReturnValue(";
    result += stringParameter;
    result += L", ";
    result += std::to_wstring(integerParameter);
    result += L") called.";
    *stringResult = SysAllocString(result.c_str());

    return S_OK;
}

STDMETHODIMP HostObjectSample::CallExtend(BSTR stringPluginName, BSTR stringMethodName, BSTR stringParameter, BSTR* stringResult)
{
    auto itFinder1 = m_pluginFunctions.find(stringPluginName);
    if (itFinder1 == m_pluginFunctions.end())
        return TYPE_E_ELEMENTNOTFOUND;
    auto itFinder2 = itFinder1->second.find(stringMethodName);
    if (itFinder2 == itFinder1->second.end())
        return TYPE_E_DLLFUNCTIONNOTFOUND;
    HRESULT res = itFinder2->second(m_appWindow, stringParameter, stringResult);
    return res;
}

STDMETHODIMP HostObjectSample::LoadPlugins(BSTR stringDllPath, BSTR* stringPluginsName)
{
    std::wstring validPath = findFile(stringDllPath);
    if (validPath.empty())
        validPath = findFile(stringDllPath + std::wstring(L".dll"));
    if (validPath.empty())
        return E_ACCESSDENIED;
    HMODULE h = LoadLibraryW(validPath.c_str());
    if (h == NULL)
        return E_ACCESSDENIED;

    //入口
    PFN_plugin_entry plugin_entry = (PFN_plugin_entry)GetProcAddress(h, "plugin_entry");
    if (plugin_entry == NULL)
        return E_NOINTERFACE;
    if (plugin_entry(m_appWindow) != 0)
        return E_NOTIMPL;

    //插件名
    PFN_plugin_name plugin_name = (PFN_plugin_name)GetProcAddress(h, "plugin_name");
    if (plugin_name == NULL)
        return E_NOINTERFACE;
    const wchar_t* pluginName = plugin_name();
    if (pluginName == NULL)
        return E_NOTIMPL;

    //函数列表
    PFN_plugin_functions plugin_functions = (PFN_plugin_functions)GetProcAddress(h, "plugin_functions");
    if (plugin_functions == NULL)
        return E_NOINTERFACE;
    const wchar_t* functionsNames = plugin_functions();
    if (functionsNames == NULL)
        return E_NOTIMPL;
    const wchar_t* p = functionsNames;
    while (1)
    {
        if (p == NULL)
        {
            break;
        }
        std::wstring fn(p);
        if (fn.empty())
        {
            break;
        }
        std::string sFn = UnicodeToAnsi(fn);
        PFN_PluginFunction plugin_function = (PFN_PluginFunction)GetProcAddress(h, sFn.c_str());
        if (plugin_function == NULL)
            return E_NOTIMPL;

        if (m_pluginFunctions.find(pluginName) == m_pluginFunctions.end())
            m_pluginFunctions.insert(std::make_pair(pluginName, std::map<std::wstring, PFN_PluginFunction>()));

        if (m_pluginFunctions[pluginName].find(fn) == m_pluginFunctions[pluginName].end())
            m_pluginFunctions[pluginName].insert(std::make_pair(fn, plugin_function));
        else
            m_pluginFunctions[pluginName][fn] = plugin_function;

        p += (int)(wcslen(p) + 1);
    }

    *stringPluginsName = SysAllocString(pluginName);

    return S_OK;
}

STDMETHODIMP HostObjectSample::LoadScript(BSTR stringFilePath)
{
    std::wstring validPath = findFile(stringFilePath);
    if (validPath.empty())
        validPath = findFile(stringFilePath + std::wstring(L".js"));
    if (validPath.empty())
        return E_ACCESSDENIED;

    const std::locale empty_locale = std::locale::empty();
    typedef std::codecvt_utf8<wchar_t> converter_type;  //std::codecvt_utf16
    const converter_type* converter = new converter_type;
    const std::locale utf8_locale = std::locale(empty_locale, converter);

    std::wifstream fin(validPath.c_str());

    std::locale oldLocale = fin.imbue(utf8_locale);

    if (fin.good())
    {
        std::wstringstream ssContent;
        ssContent << fin.rdbuf();
        std::wstring sContent = ssContent.str();

		m_appWindow->GetWebView()->ExecuteScript(sContent.c_str(),
			Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
				[this](HRESULT error, PCWSTR result) -> HRESULT
		{
			return S_OK;
		}).Get());

        fin.close();

        fin.imbue(oldLocale);
        return S_OK;
    }

    fin.imbue(oldLocale);
    
    return E_ACCESSDENIED;
}

STDMETHODIMP HostObjectSample::EvalAsync(BSTR stringScript)
{
    m_appWindow->RunAsync([this, stringScript]()->void {
        m_appWindow->GetWebView()->ExecuteScript(stringScript, 
        Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
            [this](HRESULT error, PCWSTR result) -> HRESULT
        {
            return S_OK;
        }).Get());
    });

    return S_OK;
}

static std::wstring EncodeQuote(const std::wstring& raw)
{
    std::wstring encoded;
    // Allocate 10 more chars to reduce memory re-allocation
    // due to adding potential escaping chars.
    encoded.reserve(raw.length() + 10);
    encoded.push_back(L'"');
    for (size_t i = 0; i < raw.length(); ++i)
    {
        // Escape chars as listed in https://tc39.es/ecma262/#sec-json.stringify.
        switch (raw[i])
        {
        case '\b':
            encoded.append(L"\\b");
            break;
        case '\f':
            encoded.append(L"\\f");
            break;
        case '\n':
            encoded.append(L"\\n");
            break;
        case '\r':
            encoded.append(L"\\r");
            break;
        case '\t':
            encoded.append(L"\\t");
            break;
        case '\\':
            encoded.append(L"\\\\");
            break;
        case '"':
            encoded.append(L"\\\"");
            break;
        default:
            encoded.push_back(raw[i]);
        }
    }
    encoded.push_back(L'"');
    return encoded;
}

STDMETHODIMP HostObjectSample::GetCookies(BSTR* cookiesArrayJson)
{
    std::wstring jsonStr(1024, '\0');
    jsonStr = L"[";
    const std::vector<wil::com_ptr<ICoreWebView2Cookie> >& cookies = m_appWindow->GetCookies();
    for (size_t i = 0; i < cookies.size(); i++)
    {                
        std::wstring ent(100, '\0');
        ent = L"{";
        bool bAdded = false;

        wil::unique_cotaskmem_string buff;
        if (SUCCEEDED(cookies[i]->get_Name(&buff)))
        {
            if (!bAdded)
				ent += L"\"name\":" + EncodeQuote(buff.get());
            else
                ent += L",\"name\":" + EncodeQuote(buff.get());
            bAdded = true;
        }
        
        if (SUCCEEDED(cookies[i]->get_Value(&buff)))
        {
            if (!bAdded)
                ent += L"\"value\":" + EncodeQuote(buff.get());
            else
                ent += L",\"value\":" + EncodeQuote(buff.get());
            bAdded = true;
        }

        if (SUCCEEDED(cookies[i]->get_Domain(&buff)))
        {
            if (!bAdded)
                ent += L"\"domain\":" + EncodeQuote(buff.get());
            else
                ent += L",\"domain\":" + EncodeQuote(buff.get());
            bAdded = true;
        }

        if (SUCCEEDED(cookies[i]->get_Path(&buff)))
        {
            if (!bAdded)
                ent += L"\"path\":" + EncodeQuote(buff.get());
            else
                ent += L",\"path\":" + EncodeQuote(buff.get());
            bAdded = true;
        }

        double d = 0.0;
        if (SUCCEEDED(cookies[i]->get_Expires(&d)))
        {
            if (!bAdded)
                ent += L"\"expires\":" + std::to_wstring(d);
            else
                ent += L",\"expires\":" + std::to_wstring(d);
            bAdded = true;
        }

        BOOL b = FALSE;
        if (SUCCEEDED(cookies[i]->get_IsHttpOnly(&b)))
        {
            if (!bAdded)
                ent += L"\"is_http_only\":" + std::wstring(b ? L"true" : L"false");
            else
                ent += L",\"is_http_only\":" + std::wstring(b ? L"true" : L"false");
            bAdded = true;
        }

        if (SUCCEEDED(cookies[i]->get_IsSecure(&b)))
        {
            if (!bAdded)
                ent += L"\"is_secure\":" + std::wstring(b ? L"true" : L"false");
            else
                ent += L",\"is_secure\":" + std::wstring(b ? L"true" : L"false");
            bAdded = true;
        }

        if (SUCCEEDED(cookies[i]->get_IsSession(&b)))
        {
            if (!bAdded)
                ent += L"\"is_session\":" + std::wstring(b ? L"true" : L"false");
            else
                ent += L",\"is_session\":" + std::wstring(b ? L"true" : L"false");
            bAdded = true;
        }

        ent += L"}";

        if (i == 0)
			jsonStr += ent;
        else
            jsonStr += L"," + ent;
    }

    jsonStr += L"]";

    *cookiesArrayJson = SysAllocString(jsonStr.c_str());
    return S_OK;
}

STDMETHODIMP HostObjectSample::get_Property(BSTR* stringResult)
{
    *stringResult = SysAllocString(m_propertyValue.c_str());
    return S_OK;
}

STDMETHODIMP HostObjectSample::put_Property(BSTR stringValue)
{
    m_propertyValue = stringValue;
    return S_OK;
}

STDMETHODIMP HostObjectSample::get_IndexedProperty(INT index, BSTR* stringResult)
{
    std::wstring result(L"[");
    result = result + std::to_wstring(index) + L"] is " + m_propertyValues[index] + L".";
    *stringResult = SysAllocString(result.c_str());
    return S_OK;
}

STDMETHODIMP HostObjectSample::put_IndexedProperty(INT index, BSTR stringValue)
{
    m_propertyValues[index] = stringValue;
    return S_OK;
}

STDMETHODIMP HostObjectSample::get_DateProperty(DATE* dateResult)
{
    *dateResult = m_date;
    return S_OK;
}

STDMETHODIMP HostObjectSample::put_DateProperty(DATE dateValue)
{
    m_date = dateValue;
    SYSTEMTIME systemTime;
    if (VariantTimeToSystemTime(dateValue, &systemTime))
    {
        // Save the Date and Time as strings to be able to easily check that we are getting the correct values.
        GetDateFormatEx(
            LOCALE_NAME_INVARIANT, 0 /*flags*/, &systemTime, NULL /*format*/, m_formattedDate, ARRAYSIZE(m_formattedDate), NULL /*reserved*/);
        GetTimeFormatEx(
            LOCALE_NAME_INVARIANT, 0 /*flags*/, &systemTime, NULL /*format*/, m_formattedTime, ARRAYSIZE(m_formattedTime));
    }
    return S_OK;
}

STDMETHODIMP HostObjectSample::CreateNativeDate()
{
    SYSTEMTIME systemTime;
    GetSystemTime(&systemTime);
    DATE date;
    if (SystemTimeToVariantTime(&systemTime, &date))
    {
        return put_DateProperty(date);
    }
    return E_UNEXPECTED;
}

STDMETHODIMP HostObjectSample::CallCallbackAsynchronously(IDispatch* callbackParameter)
{
    wil::com_ptr<IDispatch> callbackParameterForCapture = callbackParameter;
    m_runCallbackAsync([callbackParameterForCapture]() -> void {
        callbackParameterForCapture->Invoke(
            DISPID_UNKNOWN, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, nullptr, nullptr,
            nullptr, nullptr);
    });

    return S_OK;
}

STDMETHODIMP HostObjectSample::GetTypeInfoCount(UINT* pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP HostObjectSample::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    if (0 != iTInfo)
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    if (!m_typeLib)
    {
        OLECHAR filename[MAX_PATH];
        GetModuleFileNameW(NULL, filename, MAX_PATH);
        std::wstring wfilename(filename);
        size_t idx = wfilename.find_last_of('.');
        wfilename = wfilename.substr(0, idx) + L".tlb";
        RETURN_IF_FAILED(LoadTypeLib(wfilename.c_str(), &m_typeLib));
    }
    return m_typeLib->GetTypeInfoOfGuid(__uuidof(IHostObjectSample), ppTInfo);
}

STDMETHODIMP HostObjectSample::GetIDsOfNames(
    REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP HostObjectSample::Invoke(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
    VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    wil::com_ptr<ITypeInfo> typeInfo;
    RETURN_IF_FAILED(GetTypeInfo(0, lcid, &typeInfo));
    return typeInfo->Invoke(
        this, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

std::wstring HostObjectSample::findFile(const std::wstring& filename)
{
    WIN32_FIND_DATAW wfd = { 0 };
    if (INVALID_HANDLE_VALUE == ::FindFirstFileW(filename.c_str(), &wfd))
    {//find in exe dir
        WCHAR exeFilePath[MAX_PATH];
        GetModuleFileNameW(NULL, exeFilePath, MAX_PATH);
        std::wstring newFileName(exeFilePath);
        size_t idx = newFileName.find_last_of('\\');
        newFileName = newFileName.substr(0, idx) + L"\\" + filename;
        if (INVALID_HANDLE_VALUE != ::FindFirstFileW(newFileName.c_str(), &wfd))
            return newFileName;
    }
    else
    {
        return filename;
    }
    return L"";
}
