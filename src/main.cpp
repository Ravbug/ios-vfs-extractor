#include "extractor.hpp"
#include <cxxopts.hpp>
#include <iostream>

int main(int argc, char** argv) {

	cxxopts::Options options("ios-vfs-extract", "iOS FUFS extractor utility");
	options.add_options()
		("f,file", "Input file path", cxxopts::value<std::filesystem::path>())
		("o,output", "Ouptut directory path", cxxopts::value<std::filesystem::path>())
		("h,help", "Show help menu")
	;

	auto args = options.parse(argc, argv);
	if (args["help"].as<bool>()) {
		std::cout << options.help() << std::endl;
		return 0;
	}

	std::filesystem::path inFile = args["file"].as<decltype(inFile)>();
	std::filesystem::path outDir = args["output"].as<decltype(outDir)>();

	extractor::extract_to(inFile, outDir);
}