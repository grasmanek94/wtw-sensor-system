#pragma once

#include <cstdint>
#include <Arduino.h>

namespace constexpr_hash
{
    // FNV-1a 32bit hashing algorithm.
    inline constexpr std::uint32_t hash(char const* s, std::size_t count) {
        return count ? (hash(s, count - 1) ^ s[count - 1]) * 16777619u : 2166136261u;
    }

    inline std::uint32_t hash(const String& s) {
        return hash(s.c_str(), s.length());
    }

}    // namespace constexpr_hash

constexpr std::uint32_t operator"" _hash(char const* s, std::size_t count)
{
    return constexpr_hash::hash(s, count);
}
