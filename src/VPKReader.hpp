#pragma once

#include "pch.h"
#include <vector>

struct VPKHeader_v1
{
	uint32_t Signature = 0x55aa1234;
	uint32_t Version = 1;

	// The size, in bytes, of the directory tree
	uint32_t TreeSize;

public:
	size_t getEntryDataOffset() const {
		return TreeSize + sizeof(VPKHeader_v1);
	}

	HRESULT parse(IStream* pStream) {
		pStream->Read(&Signature, sizeof(uint32_t), NULL);
		pStream->Read(&Version, sizeof(uint32_t), NULL);
		pStream->Read(&TreeSize, sizeof(uint32_t), NULL);
		if (Signature != 0x55aa1234 || Version != 1) { //Header Signature
			return S_FALSE;
		}
		return S_OK;
	}
};


struct VPKEntry {
	std::string filePath;

	uint32_t CRC; // A 32bit CRC of the file's data.
	uint16_t PreloadBytes; // The number of bytes contained in the index file.

	// A zero based index of the archive this file's data is contained in.
	// If 0x7fff, the data follows the directory.
	uint16_t ArchiveIndex;

	// If ArchiveIndex is 0x7fff, the offset of the file data relative to the end of the directory (see the header for more details).
	// Otherwise, the offset of the data from the start of the specified archive.
	uint32_t EntryOffset;

	// If zero, the entire file is stored in the preload data.
	// Otherwise, the number of bytes stored starting at EntryOffset.
	uint32_t EntryLength;

	uint16_t Terminator;

	std::vector<std::byte> preloadData;
};


void ReadVPKEntryData(VPKEntry& entry, size_t entryDataOffset, IStream* pStream, std::vector<std::byte>& outData);

HRESULT ReadVPKEntries(std::map<std::string, VPKEntry>& entries, IStream* pStream, bool onlyImage = false);