#pragma once
#include <filesystem>

namespace extractor {
	void extract_to(const std::filesystem::path& inputfile, const std::filesystem::path& out_dir);
}