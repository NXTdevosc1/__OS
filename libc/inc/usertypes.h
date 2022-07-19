#pragma once
#include <stddef.h>

#define eXTAPI __stdcall
typedef long long WSTATUS;




typedef unsigned char UINT8;
typedef char INT8;
typedef unsigned short UINT16;
typedef short INT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned long long UINT64;
typedef long long INT64;
typedef UINT64* ULONGPTR;
typedef INT64* LONGPTR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef UINT16* LPWSTR;
typedef const UINT16* LPCWSTR;

typedef UINT64 ULONG;
typedef INT64 LONG;
typedef UINT32 UINT;
typedef INT32 INT;
typedef UINT16 USHORT;
typedef INT16 SHORT;
typedef UINT8 UCHAR;
typedef INT8 CHAR;

typedef UINT DWORD;
typedef USHORT WORD;
typedef ULONG QWORD;

typedef UCHAR BOOL;

typedef void* HANDLE;
typedef HANDLE HPROCESS;
typedef HANDLE HMODULE;
typedef HANDLE HTHREAD;
typedef HANDLE HCLIENT;

#define SUCCEEDED(x) (WSTATUS)(x) >= 0
#define STATUS_OK(x) ((WSTATUS)x >= 0 && (WSTATUS)x < 0x500000)
#define STATUS_WARNING(x) ((WSTATUS)x >= 0x500000)
#define STATUS_ERROR(x) ((WSTATUS)x < 0)
#define SUCCESS 0
#define ERROR -1