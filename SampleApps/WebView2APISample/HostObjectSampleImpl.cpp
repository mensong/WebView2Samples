// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "HostObjectSampleImpl.h"
#include "CheckFailure.h"
#include <fstream>
#include <sstream>

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
    HMODULE h = LoadLibraryW(stringDllPath);
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
    std::wifstream fin(stringFilePath);
    if (fin.good())
    {
        std::wstringstream ssContent;
        ssContent << fin.rdbuf();

        m_appWindow->GetWebView()->ExecuteScript(ssContent.str().c_str(), 
            Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [this](HRESULT error, PCWSTR result) -> HRESULT
        {
		    return S_OK;
        }).Get());

        fin.close();

        return S_OK;
    }
    
    return E_ACCESSDENIED;
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
