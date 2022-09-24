#pragma once
#ifndef NULL
#define NULL (LPVOID)0
#endif
#include <stdint.h>
#include <krnltypes.h>


extern void __fastcall _SSE_Memset(LPVOID Address, UINT64 Value, UINT64 Count);
extern void __fastcall _SSE_MemsetUnaligned(LPVOID Address, UINT64 Value, UINT64 Count);

extern void __fastcall _AVX_Memset(LPVOID Address, UINT64 Value, UINT64 Count);
extern void __fastcall _AVX_MemsetUnaligned(LPVOID Address, UINT64 Value, UINT64 Count);
LPVOID (*memset)(LPVOID ptr, UINT8 value, size_t size);



LPVOID __fastcall _Wrap_SSE_Memset(LPVOID ptr, UINT64 value, size_t size);
LPVOID __fastcall _Wrap_AVX_Memset(LPVOID ptr, UINT64 value, size_t size);

LPVOID __fastcall _r8_SSE_Memset(LPVOID ptr, uint8_t value, size_t size);
LPVOID __fastcall _r8_AVX_Memset(LPVOID ptr, uint8_t value, size_t size);

void (__fastcall *_SIMD_Memset)(LPVOID ptr, UINT64 Value, UINT64 Count);


void memset16(LPVOID ptr, uint16_t value, size_t size);
void memset32(LPVOID ptr, uint32_t value, size_t size);
void memset64(LPVOID ptr, uint64_t value, size_t size);


LPVOID memcpy(LPVOID dest, LPCVOID src, size_t size);

void memcpy16(LPVOID dest, LPCVOID src, size_t size);
void memcpy32(LPVOID dest, LPCVOID src, size_t size);
void memcpy64(LPVOID dest, LPCVOID src, size_t size);

int memcmp(LPVOID x, LPVOID y, size_t size);
#define ZeroMemory(ptr, len) memset(ptr, 0, len)
#define SZeroMemory(ptr) memset(ptr, 0, sizeof(*ptr))