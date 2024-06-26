#pragma once


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

typedef UINT64 SECURITY_DESCRIPTOR;

typedef int BOOL;
typedef unsigned char BYTE;

typedef void* HANDLE;


/*typedef struct _HANDLE_TABLE{
    UINT64 HandleId;
    UINT32 Status;
    UINT32 Type;
    LPVOID Value;
} HANDLE_TABLE, *HANDLE;*/



typedef void VOID;
typedef void* PVOID;
typedef void* LPVOID;
typedef UINT64 SIZE;

typedef HANDLE HTHREAD;
typedef HANDLE HPROCESS;

typedef void* RFTHREAD;
typedef void* RFPROCESS;

#define TRUE 1
#define FALSE 0
#define NULL (void*)0
#define KERNEL_SUCCESS 0
#define KERROR(x) x < 0

enum THREAD_CREATION_FLAGS {
    THREAD_CREATE_SUSPEND = 1,
    THREAD_CREATE_IMAGE_STACK_SIZE = 2
};

enum PROCESS_TOKEN_PRIVILEGES{
    PRIVILEGE_ALL = 0xFFFFFFFFFFFFFFFF,
    PRIVILEGE_SYSTEM_FILES = 1,
    PRIVILEGE_READ_PROCESS_MEMORY = 2,
    PRIVILEGE_WRITE_PROCESS_MEMORY = 4,
    PRIVILEGE_RAW_DISK_READ = 8,
    PRIVILEGE_RAW_DISK_WRITE = 0x10,
    PRIVILEGE_PROCESS_CREATE = 0x20,
    PRIVILEGE_THREAD_CREATE = 0x40,
    PRIVILEGE_PROCESS_TERMINATE = 0x80,
    PRIVILEGE_DUPLICATE_HANDLE = 0x100,
    PRIVILEGE_QUERY_INFORMATION = 0x200,
    PRIVILEGE_QUERY_LIMITED_INFORMATION = 0x400,
    PRIVILEGE_SUSPEND_RESUME = 0x800,
    PRIVILEGE_SET_INFORMATION = 0x1000,
    PRIVILEGE_SYNCHRONIZE = 0x2000,
    PRIVILEGE_VM_OPERATION = 0x4000,
    PRIVILEGE_VM_READ = 0x8000,
    PRIVILEGE_VM_WRITE = 0x10000,
    PRIVILEGE_REGISTRY = 0x20000,
    PRIVILEGE_REGISTER_DRIVER = 0x40000
};

enum _SUBSYSTEM{
    SUBSYSTEM_UNKNOWN = 0,
    SUBSYSTEM_NATIVE = 1,
    SUBSYSTEM_GUI = 2,
    SUBSYSTEM_CONSOLE = 3
};

#define CREATE_PROCESS_KERNEL 0x08
#define CREATE_PROCESS_USER 0x18

typedef void (__stdcall *THREAD_START_ROUTINE)();

#define __STACK_PUSH(Stack, Var) Stack = (UINT64)Stack - 8; *(UINT64*)(Stack) = (UINT64)Var

typedef long long KERNELSTATUS;

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

#define KERNEL_SERR_INCORRECT_DEVICE_CONFIGURATION -10
#define KERNEL_SERR_UNSUFFICIENT_MEMORY -11

#define KERNEL_OK(x) (x >= 0 && x < 0x100000)
#define KERNEL_DEPRECATE(x) (x >= 0x100000 && x < 0x250000)
#define KERNEL_WARNING(x) (x >= 0x250000 && x < 0x500000)
#define KERNEL_ERROR(x) (x < 0)
#define KERNEL_SUCCEEDED(x) (x >= 0)

#define KERNEL_STATUS_FATAL_ERROR(x) (x | (0xE000000000000000))

#define _IN
#define _OUT
#define _OPT