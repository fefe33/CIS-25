//this file was MOSTLY generated using claude4
//i needed a quick file for packing unsigned integers into 4 bytes (for the purposes of long term storage).
//added inline keyword (to prevent multiple definitions of same functions when compiler enters linker phase)

#pragma once

#include <cstdint>
#include <vector>

enum class Endian { LITTLE, BIG };

inline std::vector<uint8_t> packBytes(uint32_t value, size_t n, Endian order) {
    std::vector<uint8_t> bytes(n);

    for (size_t i = 0; i < n; i++) {
        uint8_t byte = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);

        if (order == Endian::LITTLE) {
            bytes[i] = byte;             // least significant byte first
        } else {
            bytes[n - 1 - i] = byte;     // most significant byte first — just reverse the index
        }
    }

    return bytes;
}

inline uint32_t unpackBytes(const std::vector<uint8_t>& bytes, Endian order) {
    uint32_t value = 0;
    size_t n = bytes.size();

    for (size_t i = 0; i < n; i++) {
        uint8_t byte;
        if (order == Endian::LITTLE) {
            byte = bytes[i];
        } else {
            byte = bytes[n - 1 - i];
        }
        value |= (static_cast<uint32_t>(byte) << (8 * i));
    }

    return value;
}
