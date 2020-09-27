#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <iomanip>
#include <iostream>    
#include <sstream> 
#include <vector>
#include "racjin.h"


namespace fs = std::filesystem;


struct Header {
	uint32_t offset = 0;
	uint32_t compressedSize = 0;
	uint16_t sectionCount = 0;
	uint16_t isCompressed = 0;
	uint32_t decompressedSize = 0;
};

int main()
{
	std::cout << "Welcome! Put CFC.DIG into this folder\n";
	std::cout << "Press any key to continue...\n";
	std::cin.get();

	fs::path p = fs::current_path() / "CFC.DIG";
	fs::create_directories(fs::current_path() / "unpacked/0");
	fs::create_directories(fs::current_path() / "unpacked/1");

	std::ifstream file(p, std::ios::binary);

	if (file.is_open()) {

		std::cout << "The file was open\n";
		std::cout << "Starting unpacking\n\n";

		file.seekg(0x10);

		std::vector<Header> fileHeaders;

		do {
			Header fileHeader;
			file.read(reinterpret_cast<char*>(&fileHeader), 0x10);
			fileHeaders.push_back(fileHeader);
		} while (fileHeaders.back().offset > 0);

		fileHeaders.pop_back();

		uint32_t fileCounter = 0;

		for (auto fileHeader : fileHeaders) {

			std::cout << "offset: " << std::hex << fileHeader.offset << "\n";
			std::cout << "compressed size : " << std::hex << fileHeader.compressedSize << "\n";
			std::cout << "section count : " << std::hex << fileHeader.sectionCount << "\n";
			std::cout << "is compressed: " << std::hex << fileHeader.isCompressed << "\n";
			std::cout << "decompressed size : " << std::hex << fileHeader.decompressedSize << "\n" << "\n";

			uint32_t fileOffset = fileHeader.offset * 0x800;

			file.seekg(fileOffset);

			if (file.tellg() == -1) {
				std::cout << "The file at the offset 0x" << fileHeader.offset << "*0x800 doesn't exist! \n\n";
				file.clear();
				continue;
			}
			std::vector<uint8_t> buffer(fileHeader.compressedSize, 0);

			file.read(reinterpret_cast<char*>(buffer.data()), fileHeader.compressedSize);


			fs::path outputPath;

			std::stringstream stream;
			stream << std::hex << fileHeader.offset;
			std::string name(stream.str());

			outputPath = fileHeader.isCompressed == 1 ? "unpacked/1/" : "unpacked/0/";
			outputPath += std::to_string(1 + fileCounter) + "_" + name + ".raw";

			std::ofstream singleFile(outputPath, std::ios::binary | std::ios::trunc);

			if (singleFile.is_open()) {

				if (fileHeader.isCompressed == 1) {
					std::vector<uint8_t> decompressedBuffer = decompress(buffer, fileHeader.decompressedSize);
					/*std::vector<uint8_t> compBuffer = compress(decompressedBuffer);*/
					singleFile.write(reinterpret_cast<char*>(decompressedBuffer.data()), fileHeader.decompressedSize);
				}
				else {
					singleFile.write(reinterpret_cast<char*>(buffer.data()), fileHeader.decompressedSize);
				}
				++fileCounter;

				singleFile.close();
			}
			else {
				std::cout << "Couldn't create the output file\n\n";
			}

		}


		file.close();

		std::cout << "Total num of files: " << std::dec << fileCounter << "\n";
	}
	else {
		std::cout << "Couldn't open CFC.DIG\n";
	}

	std::cout << "Press any key to exit...\n";

	std::cin.get();

	return 0;
}


