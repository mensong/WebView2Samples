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

typedef HRESULT(*FN_ExecuteScript)(AppWindow* appWindow, BSTR stringScript, BSTR* stringResult);
typedef void (*FN_CloseApp)(AppWindow* appWindow);

#define DEF_PROC(hDll, name) \
	name = (FN_##name)::GetProcAddress(hDll, #name)


class WebViewApi
{
public:
private:
	static WebViewApi* s_ins;

public:
	static WebViewApi& Ins()
	{
		if (!s_ins)
			s_ins = new WebViewApi;
		return *s_ins;
	}

	static void Rel()
	{
		if (s_ins)
		{
			delete s_ins;
			s_ins = NULL;
		}
	}

	WebViewApi()
	{
		OLECHAR current[MAX_PATH];
		GetModuleFileNameW(NULL, current, MAX_PATH);

		hDll = LoadLibrary(current);
		if (hDll)
		{
			HMODULE __hDll__ = (hDll);
			DEF_PROC(__hDll__, ExecuteScript);
			DEF_PROC(__hDll__, CloseApp);

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


	FN_ExecuteScript			ExecuteScript;
	FN_CloseApp					CloseApp;

	HMODULE hDll;
};
__declspec(selectany) WebViewApi* WebViewApi::s_ins = NULL;

#endif