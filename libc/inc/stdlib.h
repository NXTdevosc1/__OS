#pragma once
#include <stdint.h>
#include <stddef.h>
#include <proghdr.h>

#define MAX_CHAR_NUM 21
#define RADIX_HEXADECIMAL	0x10
#define RADIX_DECIMAL		0xA
#define RADIX_BINARY		1

#define ZeroMemory(Address, Length) memset(Address, 0, Length)
#define ObjZeroMemory(RfObject) memset((void*)RfObject, 0, sizeof(*RfObject))

_DECL void sprintf(char* buffer, const char* Format, ...);
_DECL void unumstr(uint64_t num, char* out);

_DECL char* itoa(long long _Value, char* _Buffer, int _Radix);

_DECL char* dtoa(double _Value, char* _Buffer, unsigned char DecimalSize);


_DECL void memset(void* ptr, unsigned char val, size_t size);
_DECL void _memset16(void* ptr, unsigned short val, size_t size);
_DECL void _memset32(void* ptr, unsigned int val, size_t size);
_DECL void _memset64(void* ptr, unsigned long long val, size_t size);

_DECL void memcpy(void* Destination, const void* Source, size_t size);
_DECL void _memcpy16(void* Destination, const void* Source, size_t size);
_DECL void _memcpy32(void* Destination, const void* Source, size_t size);
_DECL void _memcpy64(void* Destination, const void* Source, size_t size);

#ifdef __DLLEXPORTS
extern void __memset64(void* Destination, unsigned long long Value, unsigned long long size);
extern void __memset32(void* Destination, unsigned int Value, unsigned long long size);
extern void __memset16(void* Destination, unsigned short Value, unsigned long long size);
extern void __memset(void* Destination, unsigned char Value, unsigned long long size);
extern void _Xmemset128(void* Destination, unsigned long long Value, unsigned long long size);
#else
_DECL void __memset64(void* Destination, unsigned long long Value, unsigned long long size);
_DECL void __memset32(void* Destination, unsigned int Value, unsigned long long size);
_DECL void __memset16(void* Destination, unsigned short Value, unsigned long long size);
_DECL void __memset(void* Destination, unsigned char Value, unsigned long long size);
_DECL void _Xmemset128(void* Destination, unsigned long long Value, unsigned long long size);

#endif