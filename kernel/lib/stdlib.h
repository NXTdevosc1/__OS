#pragma once
#include <stdint.h>

#define MAKEWORD(x,y) (unsigned short)(((x & 0xFF) << 8) | y & 0xFF)
#define MAKEINT(x, y) (unsigned int)(((x & 0xFFFF) << 16) | y & 0xFFFF)
#define MAKELONG(x, y) (unsigned long long)(((x & 0xFFFFFFFF) << 32) | y & 0xFFFFFFFF)
#define SWAPWORD(x) MAKEWORD(x, x >> 8)
#define SWAPINT(x) (unsigned int)((SWAPWORD(x >> 16)) | ((SWAPWORD(x)) << 16))
#define SWAPLONG(x) (unsigned long long)((SWAPINT(x >> 32)) | ((SWAPINT(x)) << 32))

#define MAX_CHAR_NUM 21
#define RADIX_HEXADECIMAL	0x10
#define RADIX_DECIMAL		0xA
#define RADIX_BINARY		1

size_t strlen(const char* x);
unsigned int wstrlen(const wchar_t* x);
unsigned char wstrcmp(const wchar_t* x, const wchar_t* y, size_t size);
int strcmp(const char* x, const char* y);
unsigned char wstrcmp_nocs(const wchar_t* x, const wchar_t* y, size_t size); // wide char string compare no case sensitive [unicode format]
wchar_t* wstrcat(const wchar_t* x, const wchar_t* y);
char* strcat(char* dest, const char* src);
uint64_t wstrulong(wchar_t* str, uint8_t len); // todo

char* itoa(long long _Value, char* _Buffer, int _Radix);
unsigned short* itoaw(long long _Value, unsigned short* _Buffer, int _Radix);