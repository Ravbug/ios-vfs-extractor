#include "extractor.hpp"
#include <fstream>
#include <array>
#include <span>
#include <unordered_map>
#include <zlib.h>

const std::vector<std::pair<std::string_view, std::string_view>> signatureToExt{
	{"\x89PNG\r\n\x1A\n","png"},
	{"PSND","psnd"},
	{"PFNT","pfnt"},
	{"PMOD","pmod"},
	{"PIFF","piff"},
	{"LANG","lang"}
};

template<typename T>
void readBytesTo(std::istream& stream, T* dest) {
	stream.read(reinterpret_cast<char*>(dest), sizeof(T));
}

bool startsWith(const std::span<char> vec, std::string_view value) {
	return std::string_view(vec.data(), value.size()) == value;
}

std::vector<char> decompress(const std::span<char> data) {
	z_stream zs;
	std::memset(&zs, 0, sizeof(zs));
	if (inflateInit(&zs) != Z_OK) {
		throw std::runtime_error("inflateInit failed");
	}
	zs.next_in = (Bytef*)data.data();
	zs.avail_in = data.size();

	int ret = 0;
	char outbuffer[32768]{ 0 };
	std::vector<char> outData;
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);
		ret = inflate(&zs, 0);
		std::span<char> input{ outbuffer, zs.total_out - outData.size() };
		if (outData.size() < zs.total_out) {
			outData.insert(outData.end(), input.begin(),input.end());
		}
	} while (ret == Z_OK);

	inflateEnd(&zs);
	if (ret != Z_STREAM_END) {
		throw std::runtime_error("Failure during decompression");
	}

	return outData;
}

std::string_view guessExt(const std::span<char> vec) {
	for (const auto& row : signatureToExt) {
		if (startsWith(vec, row.first)) {
			return row.second;
		}
	}
	return "unknown";
}

struct VFSHeader {
	std::array<char,4>magic{ 0 };		// the file signature, "FUFS"
	uint32_t unknown{ 0 };	// not sure what this is for
	uint32_t nFiles{ 0 };	// number of files in the container

	friend std::istream& operator>>(std::istream& instream, VFSHeader& header) {
		readBytesTo(instream, &header.magic);
		readBytesTo(instream, &header.unknown);
		readBytesTo(instream, &header.nFiles);
		return instream;
	}
};

struct VFSFileTableEntry {
	uint32_t offset{ 0 };	// absolute offset of the file
	uint32_t unknown{ 0 };	// not sure what this is for, but is close to 0 in the first entry and close to UINT_MAX in the last file
	uint32_t size{ 0 };

	friend std::istream& operator>>(std::istream& instream, VFSFileTableEntry& entry) {
		readBytesTo(instream, &entry.offset);
		readBytesTo(instream, &entry.unknown);
		readBytesTo(instream, &entry.size);
		return instream;
	}
};

void extractor::extract_to(const std::filesystem::path& inputfile, const std::filesystem::path& out_dir)
{
	std::ifstream instream(inputfile, std::ios::binary);
	VFSHeader header;
	instream >> header;
	if (!instream) {
		throw std::runtime_error("Cannot open file");
	}

	// get all the file data
	std::vector< VFSFileTableEntry> files{ header.nFiles };
	for (auto& file : files) {
		instream >> file;
	}

	// extract each file
	for (uint32_t i = 0; i < files.size(); i++) {
		// get the file contents
		auto& file = files[i];
		std::vector<char> data;
		data.resize(file.size);
		instream.read(data.data(), data.size());

		auto isZipped = [](auto& data) {
			return startsWith(data, "\x78\x9C") || startsWith(data, "\x78\xDA");
		};

		// check if the file is compressed. It is if its signature begins with PLZP
		if (startsWith(data,"PLZP")) {
			// decompress and swap vectors

			auto expandedSize = *(reinterpret_cast<uint32_t*>(data.data()) + 1);
			auto compressedSize = *(reinterpret_cast<uint32_t*>(data.data()) + 2);

			auto decompressed = decompress(std::span<char>{data.data() + 12, data.size() - 12});

			// the data might be compressed a second time
			if (isZipped(decompressed)) {
				decompressed = std::move(decompress(decompressed));
			}

			data = std::move(decompressed);
		}
		else if (isZipped(data)) {
			data = std::move(decompress(data));
		}

		// guess the file extension
		auto ext = guessExt(data);

		// output the file
		auto outpath = out_dir / std::to_string(i);
		outpath.replace_extension(ext);
		std::ofstream outstream(outpath, std::ios::binary);
		outstream.write(data.data(), data.size());
	}
}
