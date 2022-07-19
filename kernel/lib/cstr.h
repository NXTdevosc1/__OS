#pragma once
#include <stdint.h>
const char* to_stringu64(uint64_t value);
const char* to_string64(int64_t value);
const char* to_hstring64(uint64_t value);
const char* to_hstring32(uint32_t value);
const char* to_hstring16(uint16_t value);
const char* to_hstring8(uint8_t value);
uint64_t utoi(const wchar_t* _str, uint8_t len); // unicode to int
const char* strdbl(double value, int radix);