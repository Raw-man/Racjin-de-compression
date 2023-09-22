#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> Decompress(const std::vector<uint8_t>& buffer, uint32_t decompressedSize);

std::vector<uint8_t> Compress(const std::vector<uint8_t>& buffer);
