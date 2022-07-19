#pragma once
#include <proghdr.h>
#include <stdint.h>
_DECL void* __cdecl malloc(size_t size);
_DECL void* __cdecl free(void* Heap);
_DECL void* __cdecl ExtendedAlloc(unsigned long long Align, size_t Size);