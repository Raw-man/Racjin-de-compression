#include <cstdint>
#include <vector>

std::vector<uint8_t> Compress(const std::vector<uint8_t>& buffer)    //LZSS based
{

	uint32_t index = 0; //position of an element from the input buffer

    uint8_t last_enc_byte = 0;//last encoded byte

    uint8_t  bit_shift = 0; //shift by bitShift (used to fold codes)

	std::vector<uint32_t> frequencies(256, 0);

    std::vector<uint32_t> seq_indices(8192, 0);

    std::vector<uint16_t> codes;

    std::vector<uint8_t> compressed_buffer;

    codes.reserve(buffer.size() / 2);

    compressed_buffer.reserve(buffer.size());


    while(index < buffer.size())
    {

        uint8_t best_freq = 0;

        uint8_t best_match = 0;

        //To get the exact same compression for CDDATA.DIG files uncomment the following:
        //if(frequencies[last_enc_byte] == 256) frequencies[last_enc_byte] = 0x00;

        uint8_t positions_to_check = frequencies[last_enc_byte] < 32 ? (frequencies[last_enc_byte] & 0x1F) : 32;

        uint32_t seq_index = index;

        for(uint8_t freq = 0; freq < positions_to_check; freq++)
        {

            uint16_t key = freq + last_enc_byte * 32; //0x1F + 0xFF*32 = 8191

            uint32_t src_index = seq_indices[key];

			uint8_t matched = 0;

            uint8_t max_length = index + 8 < buffer.size() ? 8 : buffer.size() - index;

            for(uint8_t offset = 0; offset < max_length; ++offset)
            {
                if(buffer[src_index + offset] == buffer[index + offset]) ++matched;
				else break;
			}

            if(matched > best_match)
            {
                best_freq = freq;
                best_match = matched;
			}

		}

        uint16_t code = 0x00;

        if(best_match > 0)    //found a better match?
        {
            code = code | (best_freq << 3); //f|ooooolll //f=0 (flag), o - occurrences/frequency, l -length
            code = code | (best_match - 1);   //encode a reference
            index += best_match;
		}
        else   //encode byte literal
        {
            code = 0x100 | buffer[index]; //f|bbbbbbbb //f=1
			++index;
		}

        code = code << bit_shift; //prepare for folding

        codes.push_back(code);

        ++bit_shift;

        if(bit_shift == 8) bit_shift = 0;

        uint16_t key = (frequencies[last_enc_byte] & 0x1F) + last_enc_byte * 32; //0x1F + 0xFF*32 = 8191

        seq_indices[key] = seq_index;

        frequencies[last_enc_byte] = frequencies[last_enc_byte] + 1; //increase by 1 (up to 31)

        last_enc_byte = buffer[index - 1];

	}

    //Fold codes (8 codes, 16 bytes -> 8 codes, 9 bytes)
    for(uint32_t i = 0; i < codes.size(); i = i + 8)
    {

        uint8_t group_size = i + 8 < codes.size() ? 8 : codes.size() - i;

        for(uint8_t s = 0; s <= group_size; s += 2)
        {

            uint16_t first = s > 0 ? codes[s + i - 1] : 0x00;
            uint16_t middle = s < group_size ? codes[s + i] : 0x00;
            uint16_t last = s < group_size - 1 ? codes[s + i + 1] : 0x00;

			uint16_t result = middle | (first >> 8) | (last << 8);

            compressed_buffer.push_back(result & 0xFF);

            if(s < group_size) compressed_buffer.push_back(result >> 8);


		}



	}


    return compressed_buffer;

}




