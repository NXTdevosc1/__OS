#include <sys/sys.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <fs/fs.h>
#include <interrupt_manager/SOD.h>
#include <MemoryManagement.h>
#include <sys/module.h>
#include <sys/drv.h>
#include <loaders/pe64.h>
#include <fs/fat32/fat32.h>
#include <CPU/paging.h>
#include <CPU/cpu.h>
#include <CPU/process.h>
#include <acpi/madt.h>

#define THROW_INVALID_SYSCONFIG SOD(SOD_SYSTEM_LOADING, "Invalid system config file, please reinstall operating system.")
#define CANNOT_FIND_SYSCONFIG SOD(SOD_SYSTEM_LOADING, "Cannot find system config file, please reinstall operating system.")

void* SystemSpaceBase = (void*)((UINT64)1 << 46); // Set to a higher value when 57 Bit Paging Supported
void* LocalApicPhysicalAddress = NULL;

void SysLoad(){
    
    FILE_INFORMATION_STRUCT FileInformation = { 0 };
    FILE sysconfig = OpenFile(SYS_CONFIG_PATH, FILE_OPEN_READ | FILE_OPEN_WRITE, &FileInformation);
    if (!sysconfig) CANNOT_FIND_SYSCONFIG;

    if(!FileInformation.FileSize || FileInformation.FileSize % 8) THROW_INVALID_SYSCONFIG;
    struct SYS_CONFIG_HDR* chdr = kmalloc(FileInformation.FileSize);
    if(!chdr) SET_SOD_MEMORY_MANAGEMENT;
    if (KERNEL_ERROR(ReadFile(sysconfig, 0, NULL, chdr))) SET_SOD_MEDIA_MANAGEMENT;

    if(!memcmp((void*)chdr->hdr.magic0, SYS_MAGIC0, 4) ||
    chdr->hdr.magic1 != SYS_MAGIC1 ||
    chdr->hdr.type != SFT_SYSTEM_CONFIG ||
    chdr->checksum != sizeof(struct SYS_CONFIG_HDR)
    ) THROW_INVALID_SYSCONFIG;
    
    struct SYS_CONFIG_ENTRY* entry = NULL;
    UINT64 flags = 0; /* Bit 0 : modules loaded
                    * Bit 1 : drivers loaded
                    * Bit 2 : registry loaded
                    * Bit 3 : boot configuration data loaded
                    */
    FILE_INFORMATION_STRUCT FileInfo = {0};

    // for(uint32_t i = 0;i<chdr->num_paths;i++){
    //     entry = (struct SYS_CONFIG_ENTRY*)((UINT64)chdr + chdr->paths_base + chdr->path_offsets[i]);
    //     Gp_draw_sf_textW(entry->path, 0xffffff, 500, 20 + (i * 20));
    //     switch(entry->type){
    //         case SFT_DLL:
    //         {
    //             if(!(entry->flags & 1) || flags & 1) THROW_INVALID_SYSCONFIG;
                
    //             flags |= 1;
    //             break;
    //         }
    //         case SFT_DRIVER: // InitialDrivers
    //         {
    //             // must not have flags
    //             if(entry->flags) THROW_INVALID_SYSCONFIG;
    //             FILE DriverImage = OpenFile(entry->path, FILE_OPEN_READ, &FileInfo);
    //             if(!DriverImage) THROW_INVALID_SYSCONFIG;

    //             void* ImageBuffer = kmalloc(FileInfo.FileSize);
    //             if(KERNEL_ERROR(ReadFile(DriverImage, 0, &FileInfo.FileSize, ImageBuffer))) THROW_INVALID_SYSCONFIG;
    //             if (KERNEL_ERROR(Pe64LoadNativeApplication(ImageBuffer, entry->path, entry->path))) SET_SOD_INITIALIZATION;
    //             flags |= 2;
    //             break;
    //         }
    //         case SFT_SYS_REGISTRY:
    //         {
    //             if(flags & 4) THROW_INVALID_SYSCONFIG;
    //             flags |= 4;

    //             break;
    //         }
    //         case SFT_USER:
    //         {

    //             break;
    //         }
    //         case SFT_BOOT_CONFIGURATION_DATA:
    //         {
    //             if(flags & 8) THROW_INVALID_SYSCONFIG;
    //             flags |= 8;
    //             break;
    //         }
    //         case SFT_STARTUP_PROGRAM:
    //         {
    //             /*KERNELSTATUS Status = Pe64LoadExecutableImage(entry->path, 3);

    //             if (KERNEL_ERROR(Status)) SOD(0, (char*)to_hstring64(Status));*/

    //             break;
    //         }

    //     }
    // }

    // check if system essentials are loaded

    //if((flags & 0b1111)!=0b1111) SET_SOD_MEDIA_MANAGEMENT;
}

void ConfigureSystemSpace(){
    UINT32 edx = 0, ebx = 0, ecx = 0;
    UINT VirtualAddressWidth = 48;
    UINT PhysicalAddressWidth = 0;
    // Check 57 Bit Paging Support
    CPUID_INFO CpuInfo = {0};
    __cpuidex(&CpuInfo, 7, 0);
    if((CpuInfo.ecx & 1 << 16)){
        // VirtualAddressWidth = 57;
    }
    // Check Max Physical Address Bits
    
    
    __cpuidex(&CpuInfo, 0x80000008, 0);
    PhysicalAddressWidth = CpuInfo.eax & 0xff;
    
    // If Max Physical Address Bits - Virtual Address Bits < 2 Then throw an error

    // on 57 bit paging processors, physical address is up to 52 bits
    
    if(VirtualAddressWidth == 57) {
        SystemSpaceBase = (void*)0xFF00000000000000;
        SOD(0, "57-BIT PML5 Support");
    } else if(VirtualAddressWidth == 48) {
        SystemSpaceBase = (void*)0xFFFF800000000000;
    } else SOD(0, "Unknown Virtual Address width");
    // on 48 bit paging processors, physical address is up to 46 bits
    
    
    for (UINT64 i = 0; i < Pmgrt.NumProcessors; i++) {
        MapPhysicalPages(kproc->PageMap, (void*)CpuManagementTable[i], (void*)CpuManagementTable[i], CPU_MGMT_NUM_PAGES, PM_MAP | PM_CACHE_DISABLE);
    }
    if(!InitSystemSpace(kproc)) SET_SOD_INITIALIZATION;
}

BOOL InitSystemSpace(RFPROCESS Process) {
    if (!Process) return FALSE;
    // LAPIC Does not need caching because its embedded in the processor chip which means that access to it is fast
    MapPhysicalPages(Process->PageMap, (void*)((UINT64)SystemSpaceBase + SYSTEM_SPACE_LAPIC), LocalApicPhysicalAddress, 1, PM_MAP | PM_CACHE_DISABLE);
    // Other processors will write to this, so ready or incrementing (Scheduler Cpu Time) will have a false value
    
    MapPhysicalPages(Process->PageMap, (void*)((UINT64)SystemSpaceBase + SYSTEM_SPACE_PMGRT), (void*)&Pmgrt, 0x10, PM_MAP | PM_CACHE_DISABLE);
    UINT64 NumProcessors = AcpiGetNumProcessors();
    if(!NumProcessors) return FALSE;
    for (UINT64 i = 0; i < NumProcessors; i++) {
        MapPhysicalPages(Process->PageMap, (void*)((UINT64)SystemSpaceBase + SYSTEM_SPACE_CPUMGMT + (i << CPU_MGMT_BITSHIFT)), (void*)CpuManagementTable[i], CPU_MGMT_NUM_PAGES, PM_MAP | PM_CACHE_DISABLE);
    }
    for (UINT64 i = 0; i < 0xff; i++) {
        MapPhysicalPages(Process->PageMap, (void*)((UINT64)SystemSpaceBase + SYSTEM_SPACE_INTERRUPT_HANDLERS + (i << 0xf)), GlobalWrapperPointer[i], 8, PM_MAP);
    }
    MapPhysicalPages(Process->PageMap, (void*)((UINT64)SystemSpaceBase + SYSTEM_SPACE_KERNEL), InitData.ImageBase, InitData.ImageSize >> 12, PM_MAP);
    return TRUE;
}