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

WEBVIEW_API HRESULT ExecuteScriptAsync(AppWindow* appWindow, BSTR stringScript, 
	FN_ExecuteScriptResultCallback resultCallback)
{
	appWindow->RunAsync([appWindow, stringScript, resultCallback]()->void {
		appWindow->GetWebView()->ExecuteScript(stringScript,
		Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[&](HRESULT error, PCWSTR result) -> HRESULT
		{
			if (resultCallback)
				resultCallback(error, result);
			return S_OK;
		}).Get());
	});
	
	return S_OK;
}

WEBVIEW_API HRESULT ExecuteScript(AppWindow* appWindow, BSTR stringScript, BSTR* stringResult)
{
	HRESULT res = S_OK;
	HANDLE ev = CreateEventA(NULL, TRUE, FALSE, NULL);

	appWindow->GetWebView()->ExecuteScript(stringScript,
		Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[&](HRESULT error, PCWSTR result) -> HRESULT
	{
		res = error;
		WebViewApi::SetResultString(stringResult, result);
		SetEvent(ev);
		return S_OK;
	}).Get());

	MSG msg;
	while (true)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::DispatchMessage(&msg);
			::TranslateMessage(&msg);
		}

		if (WaitForSingleObject(ev, 0) == WAIT_OBJECT_0)
			break;

		Sleep(1);
	}
	CloseHandle(ev);

	return res;
}

WEBVIEW_API void CloseApp(AppWindow* appWindow)
{
	appWindow->CloseAppWindow();
}

WEBVIEW_API HWND GetHWND(AppWindow* appWindow)
{
	return appWindow->GetMainWindow();
}
