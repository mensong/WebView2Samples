// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stdafx.h"

#include "App.h"

#include <map>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <shobjidl.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <sstream>

#include "AppWindow.h"
#include "DpiUtil.h"

HINSTANCE g_hInstance;
int g_nCmdShow;
bool g_autoTabHandle = true;
static std::map<DWORD, HANDLE> s_threads;

static int RunMessagePump();
static DWORD WINAPI ThreadProc(void* pvParam);
static void WaitForOtherThreads();

#define NEXT_PARAM_CONTAINS(command)                                                           \
    _wcsnicmp(nextParam.c_str(), command, ARRAYSIZE(command) - 1) == 0

int APIENTRY
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);
    g_nCmdShow = nCmdShow;

    // Default DPI awareness to PerMonitorV2. The commandline parameters can
    // override this.
    DPI_AWARENESS_CONTEXT dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
    std::wstring appId(L"WebView2HostApp");
    std::wstring userDataFolder(L"");
    std::wstring initialUrl;
    std::wstring startingScript;
    std::wstring loadedScript;
    std::wstring runtimefolder;
    DWORD creationModeId = IDM_CREATION_MODE_WINDOWED;
    std::vector<std::pair<std::wstring, std::wstring>> headers;
    bool hasRect = false;
    int rect_x = 0;
    int rect_y = 0;
    int rect_width = 800;
    int rect_heigth = 600;

    //MessageBoxW(0, lpCmdLine, L"command line", 0);

    if (lpCmdLine && lpCmdLine[0])
    {
        int paramCount = 0;
        LPWSTR* params = CommandLineToArgvW(GetCommandLineW(), &paramCount);
        for (int i = 0; i < paramCount; ++i)
        {
            //MessageBoxW(0, params[i], L"command", 0);

            std::wstring nextParam;
            if (params[i][0] == L'-')
            {
                if (params[i][1] == L'-')
                {
                    nextParam.assign(params[i] + 2);
                }
                else
                {
                    nextParam.assign(params[i] + 1);
                }
            }
            if (NEXT_PARAM_CONTAINS(L"dpiunaware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_UNAWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpisystemaware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpipermonitorawarev2"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;
            }
            else if (NEXT_PARAM_CONTAINS(L"dpipermonitoraware"))
            {
                dpiAwarenessContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
            }
            else if (NEXT_PARAM_CONTAINS(L"noinitialnavigation"))
            {
                initialUrl = L"none";
            }
            else if (NEXT_PARAM_CONTAINS(L"appid="))
            {
                appId = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"url="))
            {
                initialUrl = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"userdatafolder="))
            {
                userDataFolder = nextParam.substr(nextParam.find(L'=') + 1);
                if (userDataFolder.find(L":") == std::wstring::npos)
                {
                    OLECHAR curdir[MAX_PATH + 1];
                    ::GetCurrentDirectoryW(MAX_PATH, curdir);
                    userDataFolder = curdir + std::wstring(L"\\") + userDataFolder;
                }
            }
            else if (NEXT_PARAM_CONTAINS(L"runtimefolder="))
            {
                runtimefolder = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"creationmode="))
            {
                nextParam = nextParam.substr(nextParam.find(L'=') + 1);
                if (NEXT_PARAM_CONTAINS(L"windowed"))
                {
                    creationModeId = IDM_CREATION_MODE_WINDOWED;
                }
                else if (NEXT_PARAM_CONTAINS(L"visualdcomp"))
                {
                    creationModeId = IDM_CREATION_MODE_VISUAL_DCOMP;
                }
                else if (NEXT_PARAM_CONTAINS(L"targetdcomp"))
                {
                    creationModeId = IDM_CREATION_MODE_TARGET_DCOMP;
                }
#ifdef USE_WEBVIEW2_WIN10
                else if (NEXT_PARAM_CONTAINS(L"visualwincomp"))
                {
                    creationModeId = IDM_CREATION_MODE_VISUAL_WINCOMP;
                }
#endif
            }
            else if (NEXT_PARAM_CONTAINS(L"startingscript="))
            {
                startingScript = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"loadedscript="))
            {
                loadedScript = nextParam.substr(nextParam.find(L'=') + 1);
            }
            else if (NEXT_PARAM_CONTAINS(L"header="))//�ɶ�����
            {
                /* -header=Cookie=123456789 -header=User-Agent=EDGE */
                std::wstring header = nextParam.substr(nextParam.find(L'=') + 1);
                size_t idx = header.find(L'=');
                if (idx == std::wstring::npos)
                {
                    headers.push_back(std::make_pair(header, L""));
                }
                else if (idx != 0)
                {
                    headers.push_back(std::make_pair(header.substr(0, idx), header.substr(idx + 1)));
                }
            }
            else if (NEXT_PARAM_CONTAINS(L"rect="))
            {
                // -rect=x,y,width,heigth
                // -rect=100,100,800,600
                // -rect=-1,-1,800,600  ���ھ���
                std::wstring rectStr = nextParam.substr(nextParam.find(L'=') + 1);
                size_t idx = rectStr.find(L',');
                int n = 0;
				while (idx != std::wstring::npos || !rectStr.empty())
				{
                    int nv = _wtoi(rectStr.substr(0, idx).c_str());
                    switch (n++)
                    {
                    case 0:
                        rect_x = nv;
                        break;
                    case 1:
                        rect_y = nv;
                        break;
                    case 2:
                        rect_width = nv;
                        break;
                    case 3:
                        rect_heigth = nv;
                        break;
                    default:
                        break;
                    }
                    if (idx == std::wstring::npos)
                        break;
					rectStr = rectStr.substr(idx + 1);
					idx = rectStr.find(L',');
				}
                hasRect = true;
            }
        }
        LocalFree(params);
    }

    //�����û�����Ŀ¼����ʱĿ¼��
    if (userDataFolder.empty())
    {
        wchar_t tmpDir[MAX_PATH + 1] = { 0 };
        ::GetTempPathW(MAX_PATH, tmpDir);
        userDataFolder = tmpDir;
        userDataFolder += L"WebView2Host_Cache";
    }

    SetCurrentProcessExplicitAppUserModelID(appId.c_str());

    DpiUtil::SetProcessDpiAwarenessContext(dpiAwarenessContext);

    RECT winRc = { 0 };
    if (hasRect)
    {
        int nScreenWidth = GetSystemMetrics(SM_CXSCREEN); //��Ļ����
        int nScreenHeight = GetSystemMetrics(SM_CYSCREEN); //��Ļ�߶�
        if (rect_x == -1)
            rect_x = (nScreenWidth - rect_width) / 2;
        if (rect_y == -1)
            rect_y = (nScreenHeight - rect_heigth) / 2;
        winRc.left = rect_x;
        winRc.top = rect_y;
        winRc.right = rect_x + rect_width;
        winRc.bottom = rect_y + rect_heigth;
    }

    WebViewCreateOption opt;
    opt.creationModeId = creationModeId;
    opt.initialUrl = initialUrl;
    opt.startingScript = startingScript;
    opt.loadedScript = loadedScript;
    opt.userDataFolder = userDataFolder;
    opt.runtimeFolder = runtimefolder;
    opt.customWindowRect = hasRect;
	opt.windowRect = winRc;
    opt.headers = headers;

    new AppWindow(opt, true, nullptr,
#ifdef _DEBUG
        true
#else
        false
#endif
    );

    int retVal = RunMessagePump();

    WaitForOtherThreads();

    return retVal;
}

// Run the message pump for one thread.
static int RunMessagePump()
{
    HACCEL hAccelTable = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDC_WEBVIEW2HOST));

    MSG msg;

    // Main message loop:
    //! [MoveFocus0]
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            // Calling IsDialogMessage handles Tab traversal automatically. If the
            // app wants the platform to auto handle tab, then call IsDialogMessage
            // before calling TranslateMessage/DispatchMessage. If the app wants to
            // handle tabbing itself, then skip calling IsDialogMessage and call
            // TranslateMessage/DispatchMessage directly.
            if (!g_autoTabHandle || !IsDialogMessage(GetAncestor(msg.hwnd, GA_ROOT), &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    //! [MoveFocus0]

    DWORD threadId = GetCurrentThreadId();
    auto it = s_threads.find(threadId);
    if (it != s_threads.end())
    {
        CloseHandle(it->second);
        s_threads.erase(threadId);
    }

    return (int)msg.wParam;
}

// Make a new thread.
void CreateNewThread(AppWindow* app)
{
    DWORD threadId;
    app->AddRef();
    HANDLE thread = CreateThread(
        nullptr, 0, ThreadProc, reinterpret_cast<LPVOID>(app),
        STACK_SIZE_PARAM_IS_A_RESERVATION, &threadId);
    s_threads.insert(std::pair<DWORD, HANDLE>(threadId, thread));
}

// This function is the starting point for new threads. It will open a new app window.
static DWORD WINAPI ThreadProc(void* pvParam)
{
    AppWindow* app = static_cast<AppWindow*>(pvParam);
    new AppWindow(app->GetWebViewOption(), false, nullptr, app->GetShouldHaveToolbar());
    app->Release();
    return RunMessagePump();
}

// Called on the main thread.  Wait for all other threads to complete before exiting.
static void WaitForOtherThreads()
{
    while (!s_threads.empty())
    {
        std::vector<HANDLE> threadHandles;
        for (auto it = s_threads.begin(); it != s_threads.end(); ++it)
        {
            threadHandles.push_back(it->second);
        }

        HANDLE* handleArray = threadHandles.data();
        DWORD dwIndex = MsgWaitForMultipleObjects(
            static_cast<DWORD>(threadHandles.size()), threadHandles.data(), FALSE, INFINITE,
            QS_ALLEVENTS);

        if (dwIndex == WAIT_OBJECT_0 + threadHandles.size())
        {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}