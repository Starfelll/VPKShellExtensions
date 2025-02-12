#include "pch.h"
#include "ThumbnailProvider.h"
#include "MetadataProvider.h"

#include <fstream>
#include <iostream>
#include "Utils.hpp"

static LONG g_cRef = 0;


__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
	return g_cRef ? S_FALSE : S_OK;
}

STDAPI_(ULONG) DllAddRef()
{
	LONG cRef = InterlockedIncrement(&g_cRef);
	return cRef;
}

STDAPI_(ULONG) DllRelease()
{
	LONG cRef = InterlockedDecrement(&g_cRef);
	if (0 > cRef)
		cRef = 0;
	return cRef;
}


STDAPI DllRegisterServer()
{
	auto moduleName = getModuleFileName();
	REGKEY_SUBKEY_AND_VALUE keys[] = {

		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKThumbnail, nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(L"VPK Thumbnail Provider") },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKThumbnail L"\\InprocServer32", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(moduleName.c_str()) },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKThumbnail L"\\InprocServer32", L"ThreadingModel", REG_SZ, reinterpret_cast<DWORD_PTR>(L"Apartment") },
		{ HKEY_CLASSES_ROOT, L".vpk\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(szCLSID_VPKThumbnail) },


		//shell info
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" szCLSID_VPKShellInfo, nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(L"VPK Shell Info Provider") },
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" szCLSID_VPKShellInfo L"\\InprocServer32", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(moduleName.c_str()) },
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Classes\\CLSID\\" szCLSID_VPKShellInfo L"\\InprocServer32", L"ThreadingModel", REG_SZ, reinterpret_cast<DWORD_PTR>(L"Apartment") },

		// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.vpk
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.vpk", nullptr, REG_SZ, reinterpret_cast<DWORD_PTR>(szCLSID_VPKShellInfo) },
		//{ HKEY_CLASSES_ROOT, "SystemFileAssociations\\.vpk"), "ExtendedTileInfo"), REG_SZ, reinterpret_cast<DWORD_PTR>("prop:System.ItemType;VTFShellInfo.FormatName;*System.Image.Dimensions")) },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vpk", L"FullDetails", REG_SZ, reinterpret_cast<DWORD_PTR>(L"prop:System.Link.TargetParsingPath;System.Title;System.Comment;System.Author;System.ContentType;System.Document.Version;System.Link.TargetUrl;System.PropGroup.FileSystem;System.ItemNameDisplay;System.ItemType;System.ItemFolderPathDisplay;System.Size;System.DateCreated;System.DateModified;System.FileAttributes;*System.OfflineAvailability;*System.OfflineStatus;*System.SharedWith;*System.FileOwner;*System.ComputerName") },
		//{ HKEY_CLASSES_ROOT, "SystemFileAssociations\\.vpk"), "InfoTip"), REG_SZ, reinterpret_cast<DWORD_PTR>("prop:System.ItemType;VTFShellInfo.FormatName;*System.Image.Dimensions;*System.Size")) },

		// unused System.Rating;System.Keywords;
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vpk", L"PreviewDetails", REG_SZ, reinterpret_cast<DWORD_PTR>(L"prop:System.Link.TargetParsingPath;System.Title;System.Comment;System.Author;System.ContentType;System.Document.Version;System.Link.TargetUrl;*System.SharedWith;*System.OfflineAvailability;*System.OfflineStatus;*System.DateModified;*System.Size;*System.DateCreated") }
	};
	if (const auto hr = CreateRegistryKeys(keys, ARRAYSIZE(keys)); FAILED(hr))
		return hr;

	////HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\PropertySystem\PropertySchema
	//HRESULT hr = PSRegisterPropertySchema(L"C:\\Users\\nachostars\\src\\VPKShellExtensions\\VPKShellInfo.propdesc");
	//if (SUCCEEDED(hr)) {
	//	hr = PSRefreshPropertySchema();
	//}

	return S_OK;
}

STDAPI DllUnregisterServer()
{
	REGKEY_DELETEKEY keys[] = {
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKThumbnail L"\\InprocServer32" },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKThumbnail },
		{ HKEY_CLASSES_ROOT, L".vpk\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}" },

		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKShellInfo L"\\InprocServer32" },
		{ HKEY_CLASSES_ROOT, L"CLSID\\" szCLSID_VPKShellInfo },
		{ HKEY_CLASSES_ROOT, L"SystemFileAssociations\\.vpk" },
		{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\.vpk" }
	
	};
	return DeleteRegistryKeys(keys, ARRAYSIZE(keys));
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
	if (nullptr == ppv)
		return E_INVALIDARG;
	
	IClassFactory* pFactory = nullptr;
	if (IsEqualCLSID(CLSID_VPKThumbnail, rclsid)) {
		pFactory = new CVPKThumbnailProviderFactory();
	}
	else if (IsEqualCLSID(CLSID_VPKShellInfo, rclsid)) {
		pFactory = new CVPKMetadataProviderFactory();
	}
	else {
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	if (nullptr == pFactory)
		return E_OUTOFMEMORY;

	const HRESULT hr = pFactory->QueryInterface(riid, ppv);
	pFactory->Release();
	return hr;
}