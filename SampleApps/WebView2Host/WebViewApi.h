// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <string>
#include <wtypes.h>
#include <oleauto.h>

class AppWindow;

#define PLUGIN_API extern "C" __declspec(dllexport)



#ifdef WEBVIEW_PLUGINS

#define DEF_PROC(hDll, name) \
	name = (FN_##name)::GetProcAddress(hDll, #name)


class WebViewApi
{
public:
	static WebViewApi& Ins()
	{
		static WebViewApi s_ins;
		return s_ins;
	}

	typedef HRESULT(*FN_ExecuteScriptAsync)(AppWindow* appWindow, BSTR stringScript, BSTR* stringResult);
	typedef void (*FN_CloseApp)(AppWindow* appWindow);
	typedef HWND(*FN_GetHWND)(AppWindow* appWindow);

	FN_ExecuteScriptAsync		ExecuteScriptAsync;
	FN_CloseApp					CloseApp;
	FN_GetHWND					GetHWND;

private:
	WebViewApi()
	{
		OLECHAR current[MAX_PATH];
		GetModuleFileNameW(NULL, current, MAX_PATH);

		hDll = LoadLibrary(current);
		if (hDll)
		{
			HMODULE __hDll__ = (hDll);
			DEF_PROC(__hDll__, ExecuteScriptAsync);
			DEF_PROC(__hDll__, CloseApp); 
			DEF_PROC(__hDll__, GetHWND);

		}
		else
		{
			::MessageBoxA(NULL, "找不到宿主模块", "找不到模块", MB_OK | MB_ICONERROR);
		}
	}
	~WebViewApi()
	{
		if (hDll)
		{
			FreeLibrary(hDll);
			hDll = NULL;
		}
	}

	HMODULE hDll;
};

#endif