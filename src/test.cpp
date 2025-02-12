

#define UNICODE
#define _UNICODE

#include "vpkpp/vpkpp.h"
#include "kvpp/kvpp.h"
#include "windows.h"
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;




int main() {
	_wsetlocale(LC_ALL, L"chs");

	DWORD length = 512;
	WCHAR path[512];

	auto fsPath = fs::path(L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Left 4 Dead 2\\left4dead2\\addons\\[m16]2992098478.vpk");

	//if (fs::hard_link_count(fsPath) > 1) {
	//	std::wstring linkName;
	//	auto findHandle = FindFirstFileName(fsPath.c_str(), linkName);
	//	if (findHandle != INVALID_HANDLE_VALUE) {
	//		auto parentPath = fsPath.parent_path();
	//		auto fileName = fsPath.filename();

	//		auto volumeName = fs::path(GetVolumePathName(fsPath.c_str()));
	//		do {
	//			auto linkPath = volumeName / fs::path(linkName);
	//			if (linkPath != fsPath) {
	//				auto relative = linkPath.lexically_relative(parentPath);
	//				if ((!relative.empty()) && relative != fileName) {
	//					wcout << relative.c_str() << endl;
	//					break;
	//				}
	//			}
	//		} while (FindNextFileName(findHandle, linkName));
	//		FindClose(findHandle);
	//	}
	//}
	//

	return 0;
	if (auto vpk = vpkpp::VPK::open("C:/Users/nachostars/src/VPKShellExtensions/out/build/x64-Release/test1.vpk"); vpk) {
		kvpp::KV1Writer addonInfo = kvpp::KV1Writer(vpk->readEntryText("addoninfo.txt").value_or(""));

		auto& info = addonInfo[0];

		info["ex_rating"] = 32;
		vpk->removeEntry("addoninfo.txt");

		auto bakedAddonInfo = addonInfo.bake();


		auto entryOptions = vpkpp::EntryOptions();
		entryOptions.vpk_preloadBytes = vpkpp::VPK_MAX_PRELOAD_BYTES;
		entryOptions.vpk_saveToDirectory = true;
		

		vpk->addEntry("addoninfo.txt", reinterpret_cast<const std::byte*>(bakedAddonInfo.data()), bakedAddonInfo.size(), entryOptions);

		auto bakeOptions = vpkpp::BakeOptions();
		if (vpk->bake("", bakeOptions, nullptr)) {
			return 0;
		}

	}


	return 0;
}