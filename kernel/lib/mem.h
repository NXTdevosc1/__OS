#pragma once
#define NULL (void*)0
#include <stdint.h>
void* memset(void* ptr, uint8_t value, size_t size);
void memset16(void* ptr, uint16_t value, size_t size);
void memset32(void* ptr, uint32_t value, size_t size);
void memset64(void* ptr, uint64_t value, size_t size);

void* memcpy(void* dest, const void* src, size_t size);
void memcpy16(void* dest, const void* src, size_t size);
void memcpy32(void* dest, const void* src, size_t size);
void memcpy64(void* dest, const void* src, size_t size);


int memcmp(void* x, void* y, size_t size);
#define ZeroMemory(ptr, len) memset(ptr, 0, len)
#define SZeroMemory(ptr) memset(ptr, 0, sizeof(*ptr))