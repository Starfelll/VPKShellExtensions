#include "Utils.hpp"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

std::basic_string<TCHAR> getModuleFileName() {
	return GetStringFromWindowsApi<TCHAR>([](TCHAR* buffer, int size)
		{
			return GetModuleFileName(reinterpret_cast<HINSTANCE>(&__ImageBase), buffer, size);
		});
}

template <typename TChar, typename TStringGetterFunc>
std::basic_string<TChar> GetStringFromWindowsApi(TStringGetterFunc stringGetter, size_t initialSize)
{
	if (initialSize <= 0)
	{
		initialSize = MAX_PATH;
	}

	std::basic_string<TChar> result(initialSize, 0);
	for (;;)
	{
		size_t length = stringGetter(&result[0], result.length());
		if (length == 0)
		{
			return std::basic_string<TChar>();
		}

		if (length < result.length() - 1)
		{
			result.resize(length);
			result.shrink_to_fit();
			return result;
		}

		result.resize(result.length() * 2);
	}
}


static HRESULT CreateRegistryKey(REGKEY_SUBKEY_AND_VALUE* pKey)
{
	size_t cbData;
	LPVOID pvData = nullptr;
	HRESULT hr = S_OK;

	switch (pKey->dwType)
	{
	case REG_DWORD:
		pvData = reinterpret_cast<LPDWORD>(&pKey->dwData);
		cbData = sizeof(DWORD);
		break;

	case REG_SZ:
	case REG_EXPAND_SZ:
		hr = StringCbLength(reinterpret_cast<LPCWSTR>(pKey->dwData), STRSAFE_MAX_CCH, &cbData);
		if (SUCCEEDED(hr))
		{
			pvData = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(pKey->dwData));
			cbData += sizeof(WCHAR);
		}
		break;
	default:
		hr = E_INVALIDARG;
	}

	if (SUCCEEDED(hr))
	{
		const LSTATUS status = SHSetValue(pKey->hKey, pKey->lpszSubKey, pKey->lpszValue, pKey->dwType, pvData, static_cast<DWORD>(cbData));
		if (NOERROR != status)
			hr = HRESULT_FROM_WIN32(status);
	}

	return hr;
}

HRESULT CreateRegistryKeys(REGKEY_SUBKEY_AND_VALUE* aKeys, ULONG cKeys)
{
	HRESULT hr = S_OK;

	for (ULONG iKey = 0; iKey < cKeys; iKey++)
	{
		const HRESULT hrTemp = CreateRegistryKey(&aKeys[iKey]);
		if (FAILED(hrTemp))
			hr = hrTemp;
	}
	return hr;
}

HRESULT DeleteRegistryKeys(REGKEY_DELETEKEY* aKeys, ULONG cKeys)
{
	HRESULT hr = S_OK;

	for (ULONG iKey = 0; iKey < cKeys; iKey++)
	{
		const LSTATUS status = RegDeleteTree(aKeys[iKey].hKey, aKeys[iKey].lpszSubKey);
		if (NOERROR != status)
			hr = HRESULT_FROM_WIN32(status);
	}
	return hr;
}

std::string WCharToMByte(LPCWSTR lpcwszStr)
{
	std::string str;
	DWORD dwMinSize = 0;
	LPSTR lpszStr = NULL;
	dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
	if (0 == dwMinSize)
	{
		return FALSE;
	}
	lpszStr = new char[dwMinSize];
	WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwMinSize, NULL, FALSE);
	str = lpszStr;
	delete[] lpszStr;
	return str;
}

std::string WCharToUtf8(LPCWSTR lpcwszStr)
{
	std::string str;
	DWORD dwMinSize = 0;
	LPSTR lpszStr = NULL;
	dwMinSize = WideCharToMultiByte(CP_UTF8, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
	if (0 == dwMinSize)
	{
		return FALSE;
	}
	lpszStr = new char[dwMinSize];
	WideCharToMultiByte(CP_UTF8, NULL, lpcwszStr, -1, lpszStr, dwMinSize, NULL, FALSE);
	str = lpszStr;
	delete[] lpszStr;
	return str;
}

std::wstring Utf8ToWChar(LPCSTR lpcszStr)
{
	std::wstring str;
	DWORD dwMinSize = 0;
	LPWSTR lpszStr = NULL;
	dwMinSize = MultiByteToWideChar(CP_UTF8, 0, lpcszStr, -1, NULL, 0);
	if (0 == dwMinSize)
	{
		return FALSE;
	}
	lpszStr = new wchar_t[dwMinSize];
	MultiByteToWideChar(CP_UTF8, 0, lpcszStr, -1, lpszStr, dwMinSize);
	str = lpszStr;
	delete[] lpszStr;
	return str;
}