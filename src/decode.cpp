#include <cstdint>
#include <vector>

std::vector<uint8_t> Decompress(const std::vector<uint8_t>& buffer, uint32_t decompressed_size)
{

	uint32_t index = 0; //position of a byte from the input buffer
    uint32_t dest_index = 0; //position (destination) of a decoded byte in the output buffer
    uint8_t last_dec_byte = 0; //last decoded byte of the previus decoding iteration
    uint8_t  bit_shift = 0; //shift right by bitShift

	std::vector<uint8_t> frequencies(256, 0);
    std::vector<uint32_t> seq_indices(8192, 0);//references to previously decoded sequences //sliding window 32768 bytes; 2d array
    std::vector<uint8_t> decompressed_buffer(decompressed_size, 0);



    while(index < buffer.size())
    {

        uint16_t next_code = buffer[index + 1];  //next pair of bytes to decode from the input buffer
        next_code = next_code << 8;
        next_code = next_code | buffer[index];
        next_code = next_code >> bit_shift; //unfold 9 bit token

		//The result can be interpreted as follows:
		// iiiiiiif|ooooolll //f=0
		// iiiiiiif|bbbbbbbb //f=1
        //i - ignore
		//f - flag  (is literal or offset/length pair)
		//l - length (add 1 to get the real length)
		//o - occurrences/frequency
		//b - byte literal

        ++bit_shift;
		++index;

        if(bit_shift == 8)
        {
            bit_shift = 0;
			++index;
		}


        uint32_t seq_index = dest_index; //start of a byte sequence

        if((next_code & 0x100) != 0)   // bit flag: is nextToken a literal or a reference?
        {

            decompressed_buffer[dest_index] = next_code & 0xFF;//store the literal
            ++dest_index;

		}
        else
        {

            uint16_t key = ((next_code >> 3) & 0x1F) + last_dec_byte * 32; //0x1F + 0xFF*32 = 8191

            uint32_t src_index = seq_indices[key]; //get a reference to a previously decoded sequence

            for(uint8_t length = 0; length < (next_code & 0x07) + 1; ++length, ++dest_index, ++src_index)
            {

                decompressed_buffer[dest_index] = decompressed_buffer[src_index]; //copy a previously decoded byte sequence (up to 8)

			}


		}

        if(dest_index >= decompressed_buffer.size()) break;

        uint16_t key = frequencies[last_dec_byte] + last_dec_byte * 32; //0x1F + 0xFF*32 = 8191

        seq_indices[key] = seq_index;

        frequencies[last_dec_byte] = (frequencies[last_dec_byte] + 1) & 0x1F; //increase by 1 (up to 31)

        last_dec_byte = decompressed_buffer[dest_index - 1];

	}

    return decompressed_buffer;
}

