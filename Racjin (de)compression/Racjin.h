#pragma once
#include <vector>

std::vector<uint8_t> decompress(const std::vector<uint8_t>& buffer, uint32_t decompressedSize);

std::vector<uint8_t> compress(const std::vector<uint8_t>& buffer);
