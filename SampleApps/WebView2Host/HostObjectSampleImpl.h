// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "stdafx.h"

#include <functional>
#include <map>
#include <string>
#include <wrl\client.h>

#include "HostObjectSample_h.h"
#include "AppWindow.h"

//插件方法原型 
typedef HRESULT (*PFN_PluginFunction)(class AppWindow* appWindow, BSTR stringParamters, BSTR* stringResult);

//插件入口点
typedef int(*PFN_plugin_entry)(class AppWindow* appWindow);
//插件名称
typedef const wchar_t* (*PFN_plugin_name)();
//插件方法列表
typedef const wchar_t* (*PFN_plugin_functions)();

class HostObjectSample  : public Microsoft::WRL::RuntimeClass<
                            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                            IHostObjectSample, IDispatch>
{
public:
    typedef std::function<void(void)> Callback;
    typedef std::function<void(Callback)> RunCallbackAsync;

    HostObjectSample(RunCallbackAsync runCallbackAsync, AppWindow* appWindow);

    // IHostObjectSample implementation
    STDMETHODIMP MethodWithParametersAndReturnValue(
        BSTR stringParameter, INT integerParameter, BSTR* stringResult) override;

    STDMETHODIMP CallExtend(BSTR stringPluginName, BSTR stringMethodName, BSTR stringParameter, BSTR* stringResult) override;

    STDMETHODIMP LoadPlugins(BSTR stringDllPath, BSTR* stringPluginsName) override;

    STDMETHODIMP LoadScript(BSTR stringFilePath) override;

    STDMETHODIMP EvalAsync(BSTR stringScript) override;

    // Demonstrate getting and setting a property.
    STDMETHODIMP get_Property(BSTR* stringResult) override;
    STDMETHODIMP put_Property(BSTR stringValue) override;
    STDMETHODIMP get_IndexedProperty(INT index, BSTR* stringResult) override;
    STDMETHODIMP put_IndexedProperty(INT index, BSTR stringValue) override;
    STDMETHODIMP get_DateProperty(DATE* dateResult) override;
    STDMETHODIMP put_DateProperty(DATE dateValue) override;
    STDMETHODIMP CreateNativeDate() override;

    // Demonstrate native calling back into JavaScript.
    STDMETHODIMP CallCallbackAsynchronously(IDispatch* callbackParameter) override;

    // IDispatch implementation
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) override;

    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) override;

    STDMETHODIMP GetIDsOfNames(
        REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override;

    STDMETHODIMP Invoke(
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
        VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

protected:
    std::wstring findFile(const std::wstring& filename);

private:
    std::wstring m_propertyValue;
    std::map<INT, std::wstring> m_propertyValues;
    wil::com_ptr<IDispatch> m_callback;
    RunCallbackAsync m_runCallbackAsync;
    wil::com_ptr<ITypeLib> m_typeLib;
    DATE m_date;
    WCHAR m_formattedTime[200];
    WCHAR m_formattedDate[200];
    AppWindow* m_appWindow;

    //<插件名, <方法名, 方法句柄>>
    std::map<std::wstring, std::map<std::wstring, PFN_PluginFunction>> m_pluginFunctions;
};
