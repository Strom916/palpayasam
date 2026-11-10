#pragma once
#include <array>
#include <cstddef>

template <std::size_t N, char KEY>
class XorStr
{
    mutable std::array<char, N> data;

public:
    constexpr XorStr(const char(&str)[N]) : data{}
    {
        for (std::size_t i = 0; i < N; ++i)
            data[i] = str[i] ^ KEY;
    }

    const char* c_str() const
    {
        for (std::size_t i = 0; i < N; ++i)
            data[i] ^= KEY;
        return data.data();
    }

    operator const char* () const
    {
        return c_str();
    }
};

#define xorstr(str) XorStr<sizeof(str), 0x5A>(str)