#pragma once

#include "pch.h"
#include "kvpp/kvpp.h"
#include <filesystem>
namespace fs = std::filesystem;

typedef HRESULT(CALLBACK* PFN_STGOPENSTGEX)(const WCHAR*, DWORD, DWORD, DWORD, void*, void*, REFIID riid, void**);

class CVPKMetadataProviderFactory : public IClassFactory
{
private:
	LONG m_cRef;

	~CVPKMetadataProviderFactory();

public:
	CVPKMetadataProviderFactory();

	//  IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, void**);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	//  IClassFactory methods
	STDMETHOD(CreateInstance)(IUnknown*, REFIID, void**);
	STDMETHOD(LockServer)(BOOL);
};


class CVPKMetadataProvider : public IPropertyStore, IPropertyStoreCapabilities, IInitializeWithFile//IInitializeWithStream
{
	~CVPKMetadataProvider();

public:
	CVPKMetadataProvider();

	//  IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, void**);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IInitializeWithStream methods
	//STDMETHOD(Initialize)(IStream* pStream, DWORD grfMode) override;
	STDMETHOD(Initialize)(LPCWSTR pszFilePath, DWORD grfMode) override;

	// IPropertyStore methods
	STDMETHOD(GetCount)(DWORD* pcProps) override;
	STDMETHOD(GetAt)(DWORD iProp, PROPERTYKEY* pkey) override;
	STDMETHOD(GetValue)(REFPROPERTYKEY key, PROPVARIANT* pPropVar) override;
	STDMETHOD(SetValue)(REFPROPERTYKEY key, REFPROPVARIANT propVar) override;
	STDMETHOD(Commit)() override;

	// IPropertyStoreCapabilities
	STDMETHOD(IsPropertyWritable)(REFPROPERTYKEY key) override;

private:
	volatile LONG m_cRef;

	//kvpp::KV1* m_pAddonInfo = nullptr;
	std::string m_filePath;
	//DWORD m_grfMode;
	IPropertyStoreCache* m_pStore = NULL;
	//BOOL m_bReadWrite = FALSE;

	//FILE* m_pFileHandle = NULL;
	//IStream* m_pThumbStream = NULL;
	BOOL m_bHasAddonInfo = TRUE;

	template<typename T, typename U>
	HRESULT StoreIntoCache(const T& value, HRESULT(*func)(U, PROPVARIANT*), REFPROPERTYKEY key);
	//HRESULT OpenStore(BOOL bReadWrite);
};


// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AlwaysUnloadDll = 1
// HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\DesktopProcess = 1