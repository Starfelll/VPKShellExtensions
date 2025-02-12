//extern HINSTANCE hInstance;
#include "pch.h"


struct REGKEY_DELETEKEY
{
	HKEY hKey;
	LPCWSTR lpszSubKey;
};

struct REGKEY_SUBKEY_AND_VALUE
{
	HKEY hKey;
	LPCWSTR lpszSubKey;
	LPCWSTR lpszValue;
	DWORD dwType;
	DWORD_PTR dwData;
};

std::basic_string<TCHAR> getModuleFileName();

template <typename TChar, typename TStringGetterFunc>
std::basic_string<TChar> GetStringFromWindowsApi(TStringGetterFunc stringGetter, size_t initialSize = 0);

HRESULT CreateRegistryKeys(REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys);
HRESULT DeleteRegistryKeys(REGKEY_DELETEKEY* aKeys, ULONG cKeys);

std::string WCharToMByte(LPCWSTR lpcwszStr);
std::string WCharToUtf8(LPCWSTR lpcwszStr);

std::wstring Utf8ToWChar(LPCSTR lpcszStr);
inline std::wstring Utf8ToWChar(std::string_view str) {
	return Utf8ToWChar(str.data());
}