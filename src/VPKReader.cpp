#include "VPKReader.hpp"


std::string ReadString(IStream* pStream) {
	std::string result = "";
	char temp;
	while (true) {
		temp = '\0';
		pStream->Read(&temp, sizeof(temp), NULL);
		if (temp == '\0')
			return result;
		result += temp;
	}
}

void ReadVPKEntryData(VPKEntry& entry, size_t entryDataOffset, IStream* pStream, std::vector<std::byte>& outData) {
	size_t entrySize = entry.PreloadBytes + entry.EntryLength;
	outData.resize(entrySize);

	LARGE_INTEGER move;
	move.QuadPart = entryDataOffset + entry.EntryOffset;

	if (entry.PreloadBytes > 0)
		memcpy(outData.data(), entry.preloadData.data(), entry.PreloadBytes);

	pStream->Seek(move, STREAM_SEEK_SET, NULL);
	pStream->Read(outData.data() + entry.PreloadBytes, entry.EntryLength, NULL);
}


HRESULT ReadVPKEntries(std::map<std::string, VPKEntry>& entries, IStream* pStream, bool onlyImage) {
	while (true) {
		std::string ext = ReadString(pStream);
		if (ext.empty())
			break;

		while (true) {
			std::string dir = ReadString(pStream);
			if (dir.empty())
				break;

			while (true) {
				std::string filename = ReadString(pStream);
				if (filename.empty())
					break;

				VPKEntry entry;
				entry.filePath = "";
				if (dir != " ") {
					entry.filePath += dir + "/";
				}
				entry.filePath += filename;
				if (ext != " ") {
					entry.filePath += "." + ext;
				}

				pStream->Read(&entry.CRC, sizeof(uint32_t), NULL);
				pStream->Read(&entry.PreloadBytes, sizeof(uint16_t), NULL);
				pStream->Read(&entry.ArchiveIndex, sizeof(uint16_t), NULL);
				if (entry.ArchiveIndex != 0x7fff)
					return S_FALSE;

				pStream->Read(&entry.EntryOffset, sizeof(uint32_t), NULL);
				pStream->Read(&entry.EntryLength, sizeof(uint32_t), NULL);
				pStream->Read(&entry.Terminator, sizeof(uint16_t), NULL);

				if (entry.Terminator != 0xffff)
					return S_FALSE;

				if (entry.PreloadBytes > 0) {
					entry.preloadData.resize(entry.PreloadBytes);
					pStream->Read(entry.preloadData.data(), entry.PreloadBytes, NULL);
				}

				if (onlyImage) {
					if (ext == "vtf" || ext == "jpg" || ext == "png") {
						entries.emplace(entry.filePath, entry);
					}
				}
			}
		}
	}
	return S_OK;
}
