#include "WebViewApi.h"
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#ifdef USE_WEBVIEW2_WIN10
#include <pathcch.h>
#else
#include <Shlwapi.h>
#endif
#include <Psapi.h>

#include "WebViewApi.h"
#include "AppWindow.h"
#include "CheckFailure.h"

using namespace Microsoft::WRL;

#define WEBVIEW_API extern "C" __declspec(dllexport)

WEBVIEW_API HRESULT ExecuteScriptAsync(AppWindow* appWindow, BSTR stringScript, BSTR* stringResult)
{
	appWindow->RunAsync([appWindow, stringScript]()->void {
		appWindow->GetWebView()->ExecuteScript(stringScript,
		Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[](HRESULT error, PCWSTR result) -> HRESULT
		{
			return S_OK;
		}).Get());
	});
	
	return S_OK;
}

WEBVIEW_API void CloseApp(AppWindow* appWindow)
{
	appWindow->CloseAppWindow();
}

WEBVIEW_API HWND GetHWND(AppWindow* appWindow)
{
	return appWindow->GetMainWindow();
}