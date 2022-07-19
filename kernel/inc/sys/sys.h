#pragma once
#include <stdint.h>
#include <CPU/process_defs.h>
#define SYS_MAGIC0 "_SYS"
#define SYS_MAGIC1 0xFF1F496858A5FBFF
#define SYS_EXTENSION "sys"
#define SYS_CONFIG_PATH L"//OS/System/sysconfig.sys"
enum SYSF_TYPES{
    SFT_SYSTEM_CONFIG = 0,
    SFT_DLL_DIR = 1,
    SFT_DRIVER = 2,
    SFT_SYS_REGISTRY = 3,
    SFT_USER = 4,
    SFT_BOOT_CONFIGURATION_DATA = 5,
    SFT_DLL = 6,
    SFT_STARTUP_PROGRAM = 9
};
#pragma pack(push, 1)
struct SYS_CONFIG_ENTRY{
    uint8_t type;
    uint8_t flags;
    unsigned short path[];
};
struct SYS_FILE_HDR {
    uint8_t magic0[4];
    uint16_t osver_major;
    uint16_t osver_minor;
    uint64_t magic1;
    uint8_t type; // one of SYSF_TYPES
     //[num_paths] paths are 8 byte alligned
    // uint8_t type, uint8_t flags if bit 0 is set then its a folder, wchar_t paths[] starting from path_base_offset 
};
struct SYS_CONFIG_HDR{
    struct SYS_FILE_HDR hdr;
    uint8_t reserved;
    uint32_t num_paths;
    uint16_t checksum; // sizeof(struct SYS_FILE_HDR), typically 0x1C
    uint32_t paths_base;
    uint32_t path_offsets[];
};
void SysLoad();

extern void* SystemSpaceBase;
extern void* LocalApicPhysicalAddress;

#define SYSTEM_SPACE_LAPIC                0
#define SYSTEM_SPACE_PMGRT                0x100000
#define SYSTEM_SPACE_CPUMGMT              0x200000
#define SYSTEM_PROCESSORS_BUFFER          0x200000000
#define SYSTEM_READONLY_USERDATA          0xF00000000 // To avoid massive system calls and CR3 Changes (TLB Flushes)
#define SYSTEM_SPACE_INTERRUPT_HANDLERS   0x1500000000
#define SYSTEM_SPACE_KERNEL               0x2000000000

void ConfigureSystemSpace();
BOOL InitSystemSpace(RFPROCESS Process);

#pragma pack(pop)