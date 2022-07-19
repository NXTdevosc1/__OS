#pragma once
#include <krnltypes.h>
#include <kernel.h>
#define BOOTCONFIG_SIGNATURE L"Ke_$BOOTCONFIG"

#pragma pack(push, 1)
typedef struct _BOOT_CONFIGURATION_HEADER{
    UINT16 Signature[14];

    UINT32 MajorKernelVersion;
    UINT32 MinorKernelVersion;

    UINT32 MajorOsVersion;
    UINT32 MinorOsVersion;
    UINT32 Reserved0;

    UINT16 OsName[LEN_OS_NAME_MAX];
    UINT16 HeaderEnd[10];
    UINT8 Reserved1[12]; // To Ensure alignment
} BOOT_CONFIGURATION_HEADER;

typedef enum _BOOT_MODE{
    BOOT_NORMAL_MODE = 0,
    BOOT_SAFE_MODE = 1,
    BOOT_REPAIR_MODE = 2, // Not supported yet
    BOOT_MODE_MAX = 2
} BOOT_MODE;

typedef struct _BOOT_CONFIGURATION{
    BOOT_CONFIGURATION_HEADER Header;
    // Defined/Essential Boot Entries
    UINT64 BootMode; // enum BOOT_MODE
    
    UINT64 DisableMultiProcessors : 1;
    UINT64 EnableDebugging : 1;
    UINT64 DisableNetwork : 1;
    UINT64 Reserved : 62;
} BOOT_CONFIGURATION, *RFBOOT_CONFIGURATION;

#pragma pack(pop)

