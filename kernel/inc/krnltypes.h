#pragma once
#define MSABI __cdecl
#define KERNELAPI __fastcall
#define __KERNELAPI __fastcall // Applies to types

#define KEXPORT __declspec(dllexport)


#define UNITS_PER_LIST 120

#define MULTIBIT_TEST(Value, Mask) ((Value & Mask) == Mask)
#define __KERNEL

#define _IN
#define _OUT
#define _OPT

typedef int HRESULT;

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

typedef int BOOL;
typedef unsigned char BYTE;

typedef void VOID;
typedef void* LPVOID;
typedef UINT64 SIZE;

typedef INT64 KERNELSTATUS;

typedef UINT64 UINTPTR;

typedef const void* LPCVOID;

#define DWVOID (void*)(UINT64)

#define TRUE 1
#define FALSE 0

#ifndef NULL
#define NULL (void*)0
#endif

#define KERNEL_SOK 0
#define KERNEL_SOK_NOSTATUS 1

#define KERNEL_SWR_REPEATED 0x250000
#define KERNEL_SWR_RESEND	0x250001 // Tells the user to resend the message
#define KERNEL_SWR_IRQ_ALREADY_SET 0x250002 // Interrupt Handler already set by current process

#define KERNEL_SDPR_UNNECESSARY 0x100000


#define KERNEL_SERR -1
#define KERNEL_SERR_OUT_OF_RANGE -2
#define KERNEL_SERR_NOT_FOUND -3
#define KERNEL_SERR_INVALID_PARAMETER -4
#define KERNEL_SERR_UNSUPPORTED -5
#define KERNEL_SERR_FILE_OPEN_BY_ANOTHER_PROCESS -6
#define KERNEL_SERR_ADDRESS_ALREADY_TAKEN -7
#define KERNEL_SERR_INCORRECT_PASSWORD -8
#define KERNEL_SERR_IRQ_CONTROLLED_BY_ANOTHER_PROCESS -9

#define KERNEL_OK(x) (x >= 0 && x < 0x100000)
#define KERNEL_DEPRECATE(x) (x >= 0x100000 && x < 0x250000)
#define KERNEL_WARNING(x) (x >= 0x250000 && x < 0x500000)
#define KERNEL_ERROR(x) (x < 0)
#define KERNEL_SUCCEEDED(x) (x >= 0)