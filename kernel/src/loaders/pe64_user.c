#include <loaders/pe64.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <stdlib.h>
#include <fs/fs.h>
#include <MemoryManagement.h>
#include <CPU/cpu.h>
#include <kernel.h>
#include <interrupt_manager/SOD.h>
#include <loaders/subsystem.h>
#include <CPU/process.h>
#include <acpi/madt.h>
// return : 0 = success, -1 = failed to open file, -2 = invalid file format
void UnloadProgram(){
    
}

KERNELSTATUS Pe64LoadUserApplication(LPWSTR path){
    FILE_INFORMATION_STRUCT FileInfo = { 0 };
    FILE executable_file = OpenFile(path, FILE_OPEN_READ, NULL);
    if(!executable_file) return -1;
    UINT16 FileName[MAX_FILE_NAME] = {0};
    GetFileInfo(executable_file, &FileInfo, FileName);
    char* buffer = kmalloc(FileInfo.FileSize);
    if(!buffer) return -1;
    if(KERNEL_ERROR(ReadFile(executable_file, 0, NULL, buffer))) {
        CloseFile(executable_file);
        return -1;
    }
    unsigned int __pe_hdr_off = GetPEhdrOffset(buffer);
    if(__pe_hdr_off == -1 || FileInfo.FileSize + USER_IMAGE_BASE > USER_MAIN_IMAGE_LIMIT) {
        CloseFile(executable_file);
        free(buffer, kproc);
        return -2;
    }
    PE_IMAGE_HDR* image = (PE_IMAGE_HDR*)((UINT64)buffer + __pe_hdr_off);
    if (!PeCheckFileHeader(image)) {
        CloseFile(executable_file);
        free(buffer, kproc);
    }

    if (image->ThirdHeader.Subsystem == SUBSYSTEM_NATIVE) return KERNEL_SERR;

    
    RFPROCESS Process = CreateProcess(NULL, FileName, image->ThirdHeader.Subsystem, USERMODE_PROCESS);
    if(!Process){
        CloseFile(executable_file);
        free(buffer, kproc);
        return -4;
    }
    OpenHandle(Process->Handles, NULL, HANDLE_FLAG_FREE_ON_EXIT, 0, buffer, NULL);
    PE_SECTION_TABLE* Section = (PE_SECTION_TABLE*)((UINT64)&image->OptionnalHeader + image->SizeofOptionnalHeader);
    // map sections

    // THE LOADER FORCES SPACE ALIGNMENT FOR SECTION TO PREVENT HIGH MEMORY USAGE
    UINT64 ImageBase = USER_IMAGE_BASE;
    UINT64 VirtualBufferLength = 0x1000;
    {
        PE_SECTION_TABLE* s = Section;
        for (UINT16 i = 0; i < image->NumSections; i++, s++) {
            if (!(s->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) || !s->VirtualSize) continue;
            // 0x1000 Align
            if (s->VirtualSize % 0x1000) s->VirtualSize += 0x1000 - (s->VirtualSize % 0x1000);
            VirtualBufferLength += s->VirtualSize;
        }
    }



    if (!VirtualBufferLength) {
        CloseFile(executable_file);
        free(buffer, kproc);
        return -5;
    }
    char* VirtualBuffer = ExtendedMemoryAlloc(NULL, VirtualBufferLength, 0x1000, NULL, 0);
    if (!VirtualBuffer) SET_SOD_MEMORY_MANAGEMENT;

    for (UINT16 i = 0; i < image->NumSections; i++, Section++) {

        if (Section->Characteristics & (PE_SECTION_CODE | PE_SECTION_INITIALIZED_DATA | PE_SECTION_UNINITIALIZED_DATA)) {
            memcpy((void*)(VirtualBuffer + Section->VirtualAddress), (UINT64*)((char*)buffer + Section->PtrToRawData), Section->SizeofRawData);
            if (Section->VirtualSize > Section->SizeofRawData) {
                UINT64 UninitializedDataSize = Section->VirtualSize - Section->SizeofRawData;
                memset((void*)(VirtualBuffer + Section->VirtualAddress + Section->SizeofRawData), 0, UninitializedDataSize);
            }
        }
    }

    MapPhysicalPages(Process->PageMap, (LPVOID)ImageBase, VirtualBuffer, VirtualBufferLength >> 12, PM_UMAP);
    MapPhysicalPages(Process->PageMap, InitData.ImageBase, InitData.ImageBase, InitData.ImageSize >> 12, PM_MAP);
    
    // Map user read only system configuration
    MapPhysicalPages(Process->PageMap, (LPVOID)_USER_SYSTEM_CONFIG_ADDRESS, (LPVOID)&GlobalUserSystemConfig, 10, PM_PRESENT | PM_USER);

    UINT64 CpuCount = AcpiGetNumProcessors();
    UINT64 PgCount = CpuCount >> 9; // CpuCount * 4 / 0x1000

    if (CpuCount % 0x1000) PgCount++;
    
    MapPhysicalPages(Process->PageMap, (LPVOID)CpuManagementTable, (LPVOID)CpuManagementTable, PgCount, PM_MAP);

    for (UINT64 i = 0; i < CpuCount; i++) {
        if (CpuManagementTable[i]->Initialized) {
            MapPhysicalPages(Process->PageMap, (LPVOID)CpuManagementTable[i], (LPVOID)CpuManagementTable[i], 1, PM_MAP);
            MapPhysicalPages(Process->PageMap, CpuManagementTable[i]->CpuBuffer, CpuManagementTable[i]->CpuBuffer, CpuManagementTable[i]->CpuBufferSize >> 12, PM_MAP);
        }
    }

    Process->ImageHandle = image;

    HTHREAD MainThread = CreateThread(Process, image->ThirdHeader.StackReserve, (THREAD_START_ROUTINE)(ImageBase + image->OptionnalHeader.EntryPointAddr), THREAD_CREATE_SUSPEND, NULL, 0);

    if (!MainThread) SET_SOD_PROCESS_MANAGEMENT;


    UINT64 AllocationSize = image->ThirdHeader.HeapReserve;
    if (!AllocationSize) AllocationSize = PROCESS_DEFAULT_HEAP_SIZE;
    if (AllocationSize % 0x1000) AllocationSize += 0x1000 - (AllocationSize % 0x1000);

    if (!AllocateUserHeapSpace(Process, AllocationSize)) SET_SOD_PROCESS_MANAGEMENT;
    
    if (image->OptionnalDataDirectories.BaseRelocationTable.VirtualAddress) {
        if (FAILED(Pe64RelocateImage(image, buffer, VirtualBuffer, (void*)USER_IMAGE_BASE))) {
            CloseFile(executable_file);
            free(buffer, kproc);
            return -5;
        }
    }
    if (image->OptionnalDataDirectories.ImportAddressTable.VirtualAddress) {
        if(FAILED(Pe64ResolveImports(VirtualBuffer, buffer, image, Process))) {
            CloseFile(executable_file);
            free(buffer, kproc);
            return -5;
        }
    }

    if (!InitSystemSpace(Process)) SET_SOD_PROCESS_MANAGEMENT;

    ResumeThread(MainThread);
    CloseFile(executable_file);
    
    return KERNEL_SOK;
}
int PeCheckFileHeader(PE_IMAGE_HDR* hdr){
    if(hdr->MachineType != PE_TARGET_MACHINE ||
        !memcmp(hdr->Signature, PE_IMAGE_SIGNATURE, 4) ||
        hdr->SizeofOptionnalHeader < sizeof(PE_OPTIONAL_HEADER) ||
        hdr->NumSections > PE_SECTION_LIMIT ||
        hdr->ThirdHeader.Subsystem > SUBSYSTEM_MAX ||
        !(hdr->ThirdHeader.DllCharacteristics & IMAGE_DLL_DYNAMIC_BASE)
    ){
        return 0;
    }
    return 1;
}
int GetPEhdrOffset(void* hdr){
    if(!memcmp(hdr, "MZ", 2)) return -1;
    return *(int*)((char*)hdr + 0x3c); // pe offset
}