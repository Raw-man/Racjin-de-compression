#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <string>
#include <iomanip>
#include <iostream>     // std::cout
#include <sstream> 
#include <vector>

namespace fs = std::filesystem;


std::vector<uint8_t> compress(const std::vector<uint8_t>& buffer) {

	uint32_t index = 0; //position of an element from the input buffer

	uint64_t lastEncByte = 0;//last encoded byte

	uint8_t  bitShift = 0;

	std::vector<uint32_t> frequencies(256, 0);

	std::vector<uint32_t> seqIndices(8192, 0);

	std::vector<uint16_t> tokens;

	std::vector<uint8_t> compressedBuffer;

	tokens.reserve(buffer.size() / 2);

	compressedBuffer.reserve(buffer.size());


	while (index < buffer.size()) {

		uint8_t bestFreq = 0;

		uint8_t bestMatch = 0;

		uint8_t positionsToCheck = frequencies[lastEncByte] < 32 ? (frequencies[lastEncByte] & 0x1F) : 32;

		uint32_t seqIndex = index;

		for (uint8_t freq = 0; freq < positionsToCheck; freq++) {

			uint64_t key = freq + lastEncByte * 32; //0x1F + 0xFF*32 = 8191

			uint32_t srcIndex = seqIndices[key];

			uint8_t matched = 0;

			for (uint64_t offset = 0; offset < 8; ++offset) {
				if (index + offset >= buffer.size()) break;
				if (buffer[srcIndex + offset] == buffer[index + offset]) ++matched;
				else break;
			}

			if (matched > bestMatch) {
				bestFreq = freq;
				bestMatch = matched;
			}

		}

		uint16_t token = 0x00;

		if (bestMatch > 0) { //record a pair
			token = token | ((bestFreq << 3));//record frequency 
			token = token | (bestMatch - 1); //record length
			index += bestMatch;
		}
		else {
			token = 0x100 | buffer[seqIndex];
			++index;
		}

		token = token << bitShift;

		tokens.push_back(token);

		++bitShift; //prepare for packing

		if (bitShift == 8) {
			bitShift = 0;
		};


		uint64_t key = (frequencies[lastEncByte] & 0x1F) + lastEncByte * 32; //0x1F + 0xFF*32 = 8191

		seqIndices[key] = seqIndex;

		frequencies[lastEncByte] = (frequencies[lastEncByte] + 1); //increase by 1 (up to 31)

		lastEncByte = buffer[index - 1];

	}

	//Pack tokens
	for (uint64_t i = 0; i < tokens.size(); i = i + 8) {

		uint64_t groupSize = i + 8 < tokens.size() ? 8 : tokens.size() - i;

		for (uint64_t s = 0; s <= groupSize; s += 2) {

			uint16_t first = s > 0 ? tokens[s + i - 1] : 0x00;
			uint16_t middle = s < groupSize ? tokens[s + i] : 0x00;
			uint16_t last = s < groupSize - 1 ? tokens[s + i + 1] : 0x00;

			uint16_t result = middle | (first >> 8) | (last << 8);

			compressedBuffer.push_back(result & 0xFF);

			if (s < groupSize - 1 || (groupSize<8 && s<groupSize)) compressedBuffer.push_back(result >> 8);


		}



	}

	/*if (compressedBuffer.end()[-2] == 0x00 && compressedBuffer.end()[-1] == 0x00) compressedBuffer.pop_back();*/


	return compressedBuffer;

}




