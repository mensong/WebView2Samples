// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// stdafx.cpp : source file that includes just the standard includes
// WebView2Host.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

//��Ansi�ַ�ת��ΪUnicode�ַ���
std::wstring AnsiToUnicode(const std::string& multiByteStr)
{
	wchar_t* pWideCharStr; //���巵�صĿ��ַ�ָ��
	int nLenOfWideCharStr; //������ַ�������ע�ⲻ���ֽ���
	const char* pMultiByteStr = multiByteStr.c_str();
	//��ȡ���ַ��ĸ���
	nLenOfWideCharStr = MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, -1, NULL, 0);
	//��ÿ��ַ�ָ��
	pWideCharStr = (wchar_t*)(HeapAlloc(GetProcessHeap(), 0, nLenOfWideCharStr * sizeof(wchar_t)));
	MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, -1, pWideCharStr, nLenOfWideCharStr);
	//����
	std::wstring wideByteRet(pWideCharStr, nLenOfWideCharStr);
	//�����ڴ��е��ַ���
	HeapFree(GetProcessHeap(), 0, pWideCharStr);
	return wideByteRet;
}

//��Unicode�ַ�ת��ΪAnsi�ַ���
std::string UnicodeToAnsi(const std::wstring& wideByteRet)
{
	char* pMultiCharStr; //���巵�صĶ��ַ�ָ��
	int nLenOfMultiCharStr; //������ַ�������ע�ⲻ���ֽ���
	const wchar_t* pWideByteStr = wideByteRet.c_str();
	//��ȡ���ַ��ĸ���
	nLenOfMultiCharStr = WideCharToMultiByte(CP_ACP, 0, pWideByteStr, -1, NULL, 0, NULL, NULL);
	//��ö��ַ�ָ��
	pMultiCharStr = (char*)(HeapAlloc(GetProcessHeap(), 0, nLenOfMultiCharStr * sizeof(char)));
	WideCharToMultiByte(CP_ACP, 0, pWideByteStr, -1, pMultiCharStr, nLenOfMultiCharStr, NULL, NULL);
	//����
	std::string sRet(pMultiCharStr, nLenOfMultiCharStr);
	//�����ڴ��е��ַ���
	HeapFree(GetProcessHeap(), 0, pMultiCharStr);
	return sRet;
}