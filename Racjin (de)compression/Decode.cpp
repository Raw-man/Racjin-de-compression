#include <cstdint>
#include <vector>


std::vector<uint8_t> decompress(const std::vector<uint8_t>& buffer, uint32_t decompressedSize) { //LZSS based 

	uint64_t index = 0; //position of a byte from the input buffer
	uint64_t lastDecByte = 0; //last decoded byte of the previus decoding iteration
	uint32_t destIndex = 0; //position (destination) of a decoded byte in the output buffer
	uint8_t  bitShift = 0; //shift right by bitShift 

	std::vector<uint8_t> frequencies(256, 0);
	std::vector<uint64_t> seqIndices(8192, 0);//references to previously decoded sequences //sliding window 32768 bytes
	std::vector<uint8_t> decompressedBuffer(decompressedSize, 0);



	while (index < buffer.size()) {

		uint64_t nextToken = buffer[index + 1];  //next pair of bytes to decode from the input buffer
		nextToken = nextToken << 8;
		nextToken = nextToken | buffer[index];
		nextToken = nextToken >> bitShift; //unfold 9 bit token

		//The result can be interpreted as follows:
		// iiiiiiif|ooooolll //f=0
		// iiiiiiif|bbbbbbbb //f=1
		//i - ignore 
		//f - flag  (is literal or offset/length pair)
		//l - length (add 1 to get the real length)
		//o - occurrences/frequency
		//b - byte literal

		++bitShift;
		++index;

		if (bitShift == 8) {
			bitShift = 0;
			++index;
		}


		uint64_t seqIndex = destIndex; //start of a byte sequence 

		if ((nextToken & 0x100) != 0) {// bit flag: is nextToken a literal or a reference?

			decompressedBuffer[destIndex] = nextToken & 0xFF;//store the literal
			++destIndex;

		}
		else {

			uint64_t key = ((nextToken >> 3) & 0x1F) + lastDecByte * 32; //0x1F + 0xFF*32 = 8191

			uint64_t srcIndex = seqIndices[key]; //get a reference to a previously decoded sequence

			for (uint8_t length = 0; length < (nextToken & 0x07) + 1; ++length, ++destIndex, ++srcIndex) {

				decompressedBuffer[destIndex] = decompressedBuffer[srcIndex]; //copy a previously decoded byte sequence (up to 8)

			}


		}

		if (destIndex >= decompressedBuffer.size()) break;

		uint64_t key = frequencies[lastDecByte] + lastDecByte * 32; //0x1F + 0xFF*32 = 8191

		seqIndices[key] = seqIndex;

		frequencies[lastDecByte] = (frequencies[lastDecByte] + 1) & 0x1F; //increase by 1 (up to 31)

		lastDecByte = decompressedBuffer[destIndex - 1];

	}

	return decompressedBuffer;
}

