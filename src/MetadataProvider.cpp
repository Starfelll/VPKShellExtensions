#include "MetadataProvider.h"
#include "PropVariantSafe.h"
#include <propkey.h>
#include "VPKReader.hpp"
#include "Utils.hpp"
#include <vpkpp/vpkpp.h>
#include <locale>
#include <codecvt>
#include <map>
#include <set>

constexpr auto ADDONINFO = "addoninfo.txt";

//enum VPKShellInfoPropID
//{
//	Description = 114
//};
//
//// {4A327ADC-C3AC-7819-7FB6-3162280CA5FE}
//static constexpr  GUID CLSID_VPKShellInfoProps = { 0x4a327adc, 0xc3ac, 0x7819, { 0x7f, 0xb6, 0x31, 0x62, 0x28, 0x0c, 0xa5, 0xfe } };
//
//static constexpr PROPERTYKEY PKEY_VPKShellInfo_Description = PROPERTYKEY{ CLSID_VPKShellInfoProps, Description };


CVPKMetadataProvider::CVPKMetadataProvider()
{
	DllAddRef();
	m_cRef = 1;
	m_pStore = NULL;
}

CVPKMetadataProvider::~CVPKMetadataProvider()
{
	SafeRelease(&m_pStore);
	DllRelease();
}

STDMETHODIMP_(ULONG) CVPKMetadataProvider::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CVPKMetadataProvider::Release()
{
	const LONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
		delete this;
	return cRef;
}

STDMETHODIMP CVPKMetadataProvider::QueryInterface(REFIID riid, void** ppvObject)
{
	static const QITAB qit[] =
	{
		QITABENT(CVPKMetadataProvider, IPropertyStore),
		QITABENT(CVPKMetadataProvider, IPropertyStoreCapabilities),
		QITABENT(CVPKMetadataProvider, IInitializeWithFile),
		{ NULL, 0 }
	};
	return QISearch(this, qit, riid, ppvObject);
}

template <typename T, typename U>
HRESULT CVPKMetadataProvider::StoreIntoCache(const T& value, HRESULT(*func)(U, PROPVARIANT*), REFPROPERTYKEY key)
{
	PropVariantSafe variant;
	if (const auto hr = func(value, &variant.Get()); hr != S_OK)
		return hr;
	if (const auto hr = m_pStore->SetValueAndState(key, &variant.Get(), PSC_NORMAL); hr != S_OK)
		return hr;
	return S_OK;
}



inline void print_log(std::string msg) {
	//std::fstream f;
	//f.open("C:/loggg.txt", std::ios::out | std::ios::app);
	//f << msg << std::endl;
}

static HANDLE FindFirstFileName(LPCWSTR pszFilePath, std::wstring& linkName) {
	DWORD length = MAX_PATH;
	PWCHAR pString = new WCHAR[length];
	HANDLE findHandle = FindFirstFileNameW(pszFilePath, 0, &length, pString);
	if (findHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_MORE_DATA) {
		delete[] pString;
		pString = new WCHAR[length];
		findHandle = FindFirstFileNameW(pszFilePath, 0, &length, pString);
	}

	linkName = pString;
	delete[] pString;
	return findHandle;
}

static BOOL FindNextFileName(HANDLE hFindStream, std::wstring& linkName) {
	DWORD length = MAX_PATH;
	std::wstring tmp;
	tmp.resize(length);

	auto result = FindNextFileNameW(hFindStream, &length, tmp.data());

	if (result == FALSE && GetLastError() == ERROR_MORE_DATA) {
		tmp.resize(length);
		result = FindNextFileNameW(hFindStream, &length, tmp.data());
	}
	if (result == TRUE) {
		linkName = tmp.data();
	}
	return result;
}

static std::wstring GetVolumePathName(LPCWSTR pszFilePath) {
	constexpr auto size = 6;
	std::unique_ptr<WCHAR> pTmp(new WCHAR[size]);

	std::wstring result;
	if (GetVolumePathNameW(pszFilePath, pTmp.get(), size)) {
		result = pTmp.get();
	}
	return result;
}


//HRESULT CVPKMetadataProvider::Initialize(IStream* pStream, DWORD grfMode)
HRESULT CVPKMetadataProvider::Initialize(LPCWSTR pszFilePath, DWORD grfMode)
{
	if (m_pStore) {
		print_log("ERROR_ALREADY_INITIALIZED");
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	}
	else {
		if (const auto hr = PSCreateMemoryPropertyStore(IID_PPV_ARGS(&m_pStore)); FAILED(hr)) {
			print_log("PSCreateMemoryPropertyStore FAILED");
			return hr;
		}
	}

	m_filePath = std::string(WCharToMByte(pszFilePath));
	auto fsPath = fs::path(pszFilePath);
	if (fsPath.filename().wstring().starts_with(L"pak01_"))
		return S_FALSE;

	if (auto vpk = vpkpp::VPK::open(m_filePath.c_str()); vpk) {
		m_bHasAddonInfo = FALSE;
		if (auto addonInfoText = vpk->readEntryText(ADDONINFO); addonInfoText.has_value()) {
			m_bHasAddonInfo = TRUE;


			//std::string fullText;
			//vpk->runForAllEntries([&fullText](const std::string& path, const vpkpp::Entry& entry) {
			//	fullText += (path + ";");
			//});
			////fullText = "key1";
			//if (!fullText.empty()) {
			//	StoreIntoCache(Utf8ToWChar(fullText).c_str(), InitPropVariantFromStringAsVector, PKEY_Keywords);
			//}
			//PKEY_SourceItem
			//InitPropVariantFromStringAsVector()

			//StoreIntoCache(L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Left 4 Dead 2\\left4dead2\\addons\\[m16]2992098478.vpk", InitPropVariantFromString, PKEY_Link_TargetParsingPath);
	

			try {
				auto addonInfo = kvpp::KV1(addonInfoText.value());
				auto& info = (addonInfo)[0];
				
				if (auto e = info["addonTitle"]; !e.getValue().empty()) {
					StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Title);
				}
				if (auto e = info["addonVersion"]; !e.getValue().empty()) {
					StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Document_Version);
				}
				if (auto e = info["addonAuthor"]; !e.getValue().empty()) {
					StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Author);
				}
				if (auto e = info["addonDescription"]; !e.getValue().empty()) {
					StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Comment);
				}
				if (auto e = info["addonURL0"]; !e.getValue().empty()) {
					StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Link_TargetUrl);
					//// PKEY_Media_AuthorUrl
				}

				//if (SUCCEEDED(SHCreateStreamOnFileA(fsPath.replace_extension(".jpg").string().c_str(), STGM_READ, &m_pThumbStream))) {
				//	PROPVARIANT propVariant = {};
				//	propVariant.pStream = m_pThumbStream;
				//	if (SUCCEEDED(m_pStore->SetValueAndState(PKEY_ ThumbnailStream, &propVariant, PSC_READONLY))) {
				//		print_log("STTTTT");
				//	}
				//}

				//if (auto e = info["ex_rating"]; !e.isInvalid()) {
				//	StoreIntoCache(e.getValue<int>(), InitPropVariantFromUInt32, PKEY_Rating);
				//}
				//if (auto e = info["ex_keywords"]; !e.getValue().empty()) {
				//	StoreIntoCache(Utf8ToWChar(e.getValue()).c_str(), InitPropVariantFromString, PKEY_Keywords);
				//}

				std::wstring contentTypes;
				auto addContentType = [&contentTypes](std::wstring value) {
					if (!contentTypes.empty()) {
						contentTypes += L";";
					}
					contentTypes += value;
				};

				if (fs::hard_link_count(fsPath) > 1) {
					std::wstring linkName;
					auto findHandle = FindFirstFileName(fsPath.c_str(), linkName);
					if (findHandle != INVALID_HANDLE_VALUE) {
						auto parentPath = fsPath.parent_path();
						auto fileName = fsPath.filename();

						auto volumeName = fs::path(GetVolumePathName(fsPath.c_str()));
						do {
							auto linkPath = volumeName / fs::path(linkName);
							if (linkPath != fsPath) {
								auto relative = linkPath.lexically_relative(parentPath);
								if ((!relative.empty()) && relative != fileName && (!relative.begin()->wstring().starts_with(L".."))) {
									addContentType(relative);
									break;
								}
							}
						} while (FindNextFileName(findHandle, linkName));
						FindClose(findHandle);
					}
				}

				std::wstring bspList;
				BOOL hasMaps = FALSE;
				vpk->runForAllEntries("maps/",
					[&hasMaps, &bspList](const std::string& path, const vpkpp::Entry& entry) {
						if (path.ends_with(".bsp")) {
							if (!bspList.empty()) {
								bspList += L";";
							}
							bspList += Utf8ToWChar(path.substr(5, path.size() - 4 - 4 - 1));
							hasMaps = TRUE;
						}
					}
				, false);

				if (contentTypes.empty()) {
					auto isKeyOn = [&info](const char* key) {
							if (info.hasChild(key)) {
								return info[key].getValue<int>() != 0;
							}
							return false;
						};
					if (hasMaps || isKeyOn("addonContent_Campaign") || isKeyOn("addonContent_Map")) {
						addContentType(L"Campaigns");
					}
					if (isKeyOn("addonContent_weapon")) {
						addContentType(L"Weapons");
					}
					if (isKeyOn("addonContent_BossInfected") || isKeyOn("addonContent_CommonInfected")) {
						addContentType(L"Infected");
					}
					if (isKeyOn("addonContent_Survivor")) {
						addContentType(L"Survivors");
					}
					if (isKeyOn("addonContent_prop")) {
						addContentType(L"Items");
					}
					if (isKeyOn("addonContent_Sound") || isKeyOn("addonContent_Music")) {
						addContentType(L"Sounds");
					}
					if (isKeyOn("addonContent_Script")) {
						addContentType(L"Scripts");
					}
					if (isKeyOn("addonContent_Skin")) {
						addContentType(L"Skins");
					}
				}

				if (!contentTypes.empty()) {
					StoreIntoCache(contentTypes.c_str(), InitPropVariantFromString, PKEY_ContentType);
				}

				if (!bspList.empty()) {
					StoreIntoCache(bspList.c_str(), InitPropVariantFromString, PKEY_Media_SubTitle);
				}

			}
			catch (...) {
				//return S_FALSE;
			}
		}

		vpk.release();
	}
	else {
		return S_OK;
	}

	return S_OK;
}

HRESULT CVPKMetadataProvider::GetCount(DWORD* pcProps)
{
	*pcProps = 0;
	return m_pStore ? m_pStore->GetCount(pcProps) : E_UNEXPECTED;
}

HRESULT CVPKMetadataProvider::GetAt(DWORD iProp, PROPERTYKEY* pkey)
{
	*pkey = PKEY_Null;
	return m_pStore ? m_pStore->GetAt(iProp, pkey) : E_UNEXPECTED;
}

HRESULT CVPKMetadataProvider::GetValue(REFPROPERTYKEY key, PROPVARIANT* pPropVar)
{
	PropVariantInit(pPropVar);
	return m_pStore ? m_pStore->GetValue(key, pPropVar) : E_UNEXPECTED;
}

HRESULT CVPKMetadataProvider::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propVar)
{
	HRESULT hr = S_FALSE;
	if (m_pStore) {
		if (!m_bHasAddonInfo || IsEqualPropertyKey(key, PKEY_Rating) || IsEqualPropertyKey(key, PKEY_Keywords)) {
			hr = m_pStore->SetValueAndState(key, &propVar, PSC_DIRTY);
		}
		//else if (IsEqualPropertyKey(key, PKEY_Comment) || IsEqualPropertyKey(key, PKEY_Link_TargetUrl)) {
		//	hr = S_FALSE;
		//}
	}
	return hr;
}

HRESULT CVPKMetadataProvider::Commit()
{
	if (m_bHasAddonInfo) {
		return S_OK;
	}
	constexpr auto emptyAddonInfoText = "AddonInfo\n{\n}";
	HRESULT hr = E_UNEXPECTED;
	if (m_pStore) {
		//hr = STG_E_ACCESSDENIED;

		DWORD propCount = 0;
		BOOL isDirty = FALSE;
		PSC_STATE state;
		m_pStore->GetCount(&propCount);
		for (int i = 0; i < propCount; i++) {
			PROPERTYKEY key;
			m_pStore->GetAt(i, &key);
			m_pStore->GetState(key, &state);
			if (state == PSC_DIRTY) {
				isDirty = TRUE;
				break;
			}
		}
		if (!isDirty)
			return S_FALSE;

		if (!m_filePath.empty()) {
			try {
				if (auto vpk = vpkpp::VPK::open(m_filePath); vpk) {
					kvpp::KV1Writer addonInfo = kvpp::KV1Writer(m_bHasAddonInfo ? vpk->readEntryText(ADDONINFO).value_or(emptyAddonInfoText): emptyAddonInfoText);

					auto& info = addonInfo[0];
					PROPVARIANT val = {};
					
					//if (auto hr2 = m_pStore->GetValueAndState(PKEY_Rating, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
					//	info["ex_rating"] = (int)(val.uintVal);
					//}
					//if (auto hr2 = m_pStore->GetValueAndState(PKEY_Keywords, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
					//	info["ex_keywords"] = WCharToMByte(val.bstrVal);
					//}

					if (!m_bHasAddonInfo) {
						if (auto hr2 = m_pStore->GetValueAndState(PKEY_Title, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
							info["addonTitle"] = WCharToUtf8(val.pwszVal);
						}
						if (auto hr2 = m_pStore->GetValueAndState(PKEY_Document_Version, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
							info["addonVersion"] = WCharToUtf8(val.pwszVal);
						}
						if (auto hr2 = m_pStore->GetValueAndState(PKEY_Author, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
							std::wstring author = L"";
							auto elemCount = PropVariantGetElementCount(val);
							for (ULONG i = 0; i < elemCount; i++) {
								PWSTR pwstr;
								if (SUCCEEDED(PropVariantGetStringElem(val, i, &pwstr))) {
									author += pwstr;
									CoTaskMemFree(pwstr);
								}
							}
							info["addonAuthor"] = WCharToUtf8(author.c_str());
						}
						if (auto hr2 = m_pStore->GetValueAndState(PKEY_Comment, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
							info["addonDescription"] = WCharToUtf8(val.pwszVal);
						}
						if (auto hr2 = m_pStore->GetValueAndState(PKEY_Link_TargetUrl, &val, &state); SUCCEEDED(hr2) && state == PSC_DIRTY) {
							info["addonURL0"] = WCharToUtf8(val.pwszVal);
						}
					}

					vpk->removeEntry(ADDONINFO);
					auto bakedAddonInfo = addonInfo.bake();
					vpkpp::EntryOptions options{
						.vpk_preloadBytes = vpkpp::VPK_MAX_PRELOAD_BYTES,
						.vpk_saveToDirectory = true
					};

					auto addonInfoReader = std::span(reinterpret_cast<std::byte*>(bakedAddonInfo.data()), bakedAddonInfo.size());
					vpk->addEntry(ADDONINFO, addonInfoReader, options);

					if (vpk->bake("", vpkpp::BakeOptions(), nullptr)) {
						m_pStore->Commit();
						hr = S_OK;
					}
				}
			}
			catch (...) {
			}
			
		}
	}
	return hr;
}

HRESULT CVPKMetadataProvider::IsPropertyWritable(REFPROPERTYKEY key)
{
	if (!m_bHasAddonInfo) {
		if (IsEqualPropertyKey(key, PKEY_ContentType)) {
			return S_FALSE;
		}
		return S_OK;
	}

	HRESULT hr = S_FALSE;
	if (IsEqualPropertyKey(key, PKEY_Rating)) {
		hr = S_OK;
	}
	else if (IsEqualPropertyKey(key, PKEY_Comment)) {
		PSC_STATE state;
		if (SUCCEEDED(m_pStore->GetState(PKEY_Comment, &state))) {
			hr = S_OK;
		}
	}
	else if (IsEqualPropertyKey(key, PKEY_Link_TargetUrl)) {
		PSC_STATE state;
		if (SUCCEEDED(m_pStore->GetState(PKEY_Link_TargetUrl, &state))) {
			hr = S_OK;
		}
	}
	else if (IsEqualPropertyKey(key, PKEY_Media_SubTitle)) {
		PSC_STATE state;
		if (SUCCEEDED(m_pStore->GetState(PKEY_Media_SubTitle, &state))) {
			hr = S_OK;
		}
	}
	return hr;
}



STDAPI CVPKMetadataProvider_CreateInstance(REFIID riid, void** ppvObject)
{
	*ppvObject = nullptr;

	CVPKMetadataProvider* ptp = new CVPKMetadataProvider();
	if (!ptp)
		return E_OUTOFMEMORY;

	const HRESULT hr = ptp->QueryInterface(riid, ppvObject);
	ptp->Release();
	return hr;
}



/// <summary>
/// Factory
/// </summary>

CVPKMetadataProviderFactory::CVPKMetadataProviderFactory()
{
    m_cRef = 1;
    DllAddRef();
}

CVPKMetadataProviderFactory::~CVPKMetadataProviderFactory()
{
    DllRelease();
}

STDMETHODIMP_(ULONG) CVPKMetadataProviderFactory::AddRef()
{
	const LONG cRef = InterlockedIncrement(&m_cRef);
	return cRef;
}

STDMETHODIMP_(ULONG) CVPKMetadataProviderFactory::Release()
{
	const LONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
		delete this;
	return cRef;
}

STDMETHODIMP CVPKMetadataProviderFactory::QueryInterface( REFIID riid, void** ppvObject )
{
	static const QITAB qit[] =
	{
		QITABENT( CVPKMetadataProviderFactory, IClassFactory ),
		{ nullptr, 0 }
    };
	return QISearch( this, qit, riid, ppvObject );
}

STDMETHODIMP CVPKMetadataProviderFactory::CreateInstance( IUnknown* punkOuter, REFIID riid, void** ppvObject )
{
	if ( nullptr != punkOuter )
        return CLASS_E_NOAGGREGATION;

	return CVPKMetadataProvider_CreateInstance( riid, ppvObject );
}

STDMETHODIMP CVPKMetadataProviderFactory::LockServer( BOOL fLock )
{
    return E_NOTIMPL;
}
