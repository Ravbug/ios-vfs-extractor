#include "extractor.hpp"
#include <fstream>
#include <array>
#include <span>
#include <unordered_map>

const std::vector<std::pair<std::string_view, std::string_view>> signatureToExt{
	{"\x89PNG\r\n\x1A\n","png"},
	{"PSND","psnd"},
	{"PFNT","pfnt"}
};

template<typename T>
void readBytesTo(std::istream& stream, T* dest) {
	stream.read(reinterpret_cast<char*>(dest), sizeof(T));
}

bool startsWith(const std::span<char> vec, std::string_view value) {
	return std::string_view(vec.data(), value.size()) == value;

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

		// check if the file is compressed. It is if its signature begins with PLZP
		if (startsWith(data,"PLZP")) {
			// decompress and swap vectors
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
