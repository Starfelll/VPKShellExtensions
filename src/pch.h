// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <thumbcache.h>
#include <strsafe.h>
#include <string>
#include <map>
#include <tchar.h>

STDAPI_(ULONG) DllAddRef();
STDAPI_(ULONG) DllRelease();

#define szCLSID_VPKThumbnail L"{1FA157E9-03B1-44C4-074F-8997D6602032}"
static constexpr const GUID CLSID_VPKThumbnail = {0x1fa157e9, 0x03b1, 0x44c4, {0x07, 0x4f, 0x89, 0x97, 0xd6, 0x60, 0x20, 0x32 }};

#define szCLSID_VPKShellInfo L"{4D90EBFD-38BF-EF46-4ED5-9706837AF286}"
static constexpr const GUID CLSID_VPKShellInfo = {0x4d90ebfd, 0x38bf, 0xef46, {0x4e, 0xd5, 0x97, 0x06, 0x83, 0x7a, 0xf2, 0x86 }};

//#define szCLSID_VPKShellInfo L"{D06391EE-2FEB-419B-9667-AD160D0849F3}"
//static constexpr const GUID CLSID_VPKShellInfo = { 0xd06391ee, 0x2feb, 0x419b, {0x96, 0x67, 0xad, 0x16, 0x0d, 0x08, 0x49, 0xf3 } };

#define MSGBOX(TEXT) MessageBox(nullptr, TEXT, L"", 0)

template <class T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


#endif //PCH_H
