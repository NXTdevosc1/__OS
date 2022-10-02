#include <fs/fs.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <stdlib.h>
#include <dsk/ata.h>
#include <fs/mbr.h>

#include <fs/fat32/fat32.h>


KERNELSTATUS KERNELAPI GetFileInfo(FILE File, FILE_INFORMATION_STRUCT* FileInformation, LPWSTR FileName) {
    if (!File || !FileInformation) return KERNEL_SERR_INVALID_PARAMETER;
    
    if (File->Partition->Interface.GetInfo) {
        return File->Partition->Interface.GetInfo(File, FileInformation, FileName);
    }
    else return KERNEL_SERR_UNSUPPORTED;
}
KERNELSTATUS KERNELAPI ReadFile(FILE File, UINT64 Offset, UINT64* NumBytesRead, void* Buffer) {
    if (!File || !Buffer) return KERNEL_SERR_INVALID_PARAMETER;
    
    if (File->Partition->Interface.Read) {
        return File->Partition->Interface.Read(File, Offset, NumBytesRead, Buffer);
    }
    else return KERNEL_SERR_UNSUPPORTED;
}


struct VIRTUAL_PARTITION_INSTANCE* FsResolvePath(wchar_t* path, wchar_t** out){
    return NULL;
}

KERNELSTATUS KERNELAPI ListFileContent(FILE File, DIRECTORY_FILE_LIST* FileList, UINT64 MaxFileCount) {
    if (!File || !FileList) return KERNEL_SERR_INVALID_PARAMETER;
    if (!MaxFileCount) MaxFileCount = 0xFFFFFFFFFFFFFFFF;

    if (File->Partition->Interface.ListFileContent) {
        return File->Partition->Interface.ListFileContent(File, FileList, MaxFileCount);
    }
    else return KERNEL_SERR_UNSUPPORTED;
}



FILE KERNELAPI OpenFile(LPWSTR Path, UINT64 Access, FILE_INFORMATION_STRUCT* FileInformation) {
    
    if (!Path || !Access) return NULL;

    PARTITION_INSTANCE* Partition = ResolvePartition(Path, &Path);
    if (!Partition || !Partition->Interface.Open) return NULL;
    return Partition->Interface.Open(Partition, Path, Access, FileInformation);
}
KERNELSTATUS KERNELAPI CloseFile(FILE File) {
    if (!ValidateFile(File)) return KERNEL_SERR_INVALID_PARAMETER;
    if (!File->Partition->Interface.Close) return KERNEL_SERR_UNSUPPORTED;
    return File->Partition->Interface.Close(File);
}

KERNELSTATUS KERNELAPI FsReadDrive(DISK_DEVICE_INSTANCE* Drive, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer) {
    if (!Buffer || !ValidateDisk(Drive)) return KERNEL_SERR_INVALID_PARAMETER;
    DISK_MANAGEMENT_INTERFACE* Interface = (DISK_MANAGEMENT_INTERFACE*)Drive->Device->ControlInterface;
    if (!Interface || !Interface->Read) return KERNEL_SERR_UNSUPPORTED;

    return Interface->Read(Drive, SectorOffset, SectorCount, Buffer);
}
KERNELSTATUS KERNELAPI FsReadPartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer) {
    if (!Buffer || !ValidatePartition(Partition)) return KERNEL_SERR_INVALID_PARAMETER;
    if (!Partition->Interface.SectorRawRead) return KERNEL_SERR_UNSUPPORTED;
    return Partition->Interface.SectorRawRead(Partition, SectorOffset, SectorCount, Buffer);
}
KERNELSTATUS KERNELAPI FsReadPartition(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 ClusterCount, void* Buffer) {
    if (!Buffer || !ValidatePartition(Partition)) return KERNEL_SERR_INVALID_PARAMETER;
    if (!Partition->Interface.RawRead) return KERNEL_SERR_UNSUPPORTED;
    return Partition->Interface.RawRead(Partition, ClusterOffset, ClusterCount, Buffer);
}

KERNELSTATUS KERNELAPI DefaultReadPartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer) {
    DISK_MANAGEMENT_INTERFACE* Interface = Partition->Disk->Device->ControlInterface;
    return Interface->Read(Partition->Disk, Partition->PartitionInformation.BaseAddress + SectorOffset, SectorCount, Buffer);
}
KERNELSTATUS KERNELAPI DefaultWritePartitionSectors(PARTITION_INSTANCE* Partition, UINT64 SectorOffset, UINT64 SectorCount, void* Buffer) {
    DISK_MANAGEMENT_INTERFACE* Interface = Partition->Disk->Device->ControlInterface;
    return Interface->Write(Partition->Disk, Partition->PartitionInformation.BaseAddress + SectorOffset, SectorCount, Buffer);
}
FILE KERNELAPI FindOpenFilePath(OPEN_FILE_LIST* List, LPWSTR Path) {
    if (!List || !Path) return NULL;
    UINT32 Len = wstrlen(Path);
    if (!Len) return NULL; // BOOLEAN_ERROR
    for (;;) {
        for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
            if (List->Files[i].Open && List->Files[i].PathLength == Len &&
                wstrcmp_nocs(List->Files[i].Path, Path, Len)
                ) {
                return &List->Files[i];
            }
        }
        if (!List->Next) break;
        List = List->Next;
    }
    return NULL;
}
FILE KERNELAPI DefaultOpenFile(PARTITION_INSTANCE* Partition, LPWSTR Path, UINT64 Access, FILE_INFORMATION_STRUCT* FileInformation) {
    if (!ValidatePartition(Partition) || !Path) return NULL;
    if (!Partition->Interface.GetInfo) return NULL;
    UINT64 PathLength = wstrlen(Path);
    RFPROCESS Process = KeGetCurrentProcess();
    LPWSTR Copy = AllocatePool((PathLength + 1) << 1);
    memcpy16(Copy, Path, PathLength);
    Copy[PathLength] = 0;

    if (!FixFilePath(Copy)) return NULL;
    
    if ((Access & (FILE_OPEN_WRITE | FILE_OPEN_SET_INFORMATION))) {
        FILE FoundFile = NULL;
        if ((FoundFile = FindOpenFilePath(&Partition->FileList, Copy))) {
            if (FoundFile->Process == Process) {
                FoundFile->OpenAccess |= Access; // Add the new privileges
                return FoundFile;
            }
            if ((Access & FILE_OPEN_WRITE) && (FoundFile->OpenAccess & FILE_OPEN_WRITE)) {
                RemoteFreePool(kproc, Copy);
                return NULL;
            }
            if ((Access & FILE_OPEN_SET_INFORMATION) && (FoundFile->OpenAccess & FILE_OPEN_SET_INFORMATION)) {
                RemoteFreePool(kproc, Copy);
                return NULL;
            }
        }
    }

    FILE File = AllocateFile(&Partition->FileList);
    File->OpenAccess = (DWORD)Access;
    File->Partition = Partition;
    File->Path = Copy;
    File->PathLength = wstrlen(Copy);
    File->Process = Process;
    FILE_INFORMATION_STRUCT _FileInfo = { 0 };
    BOOL FI = FALSE;
    if (!FileInformation) {
        FileInformation = &_FileInfo;
    }
    

    if (KERNEL_ERROR(GetFileInfo(File, FileInformation, NULL)))
    {
        ReleaseFile(File);
        return NULL;
    }

    // After success create file handle

    HANDLE FileHandle = OpenHandle(File->Process->FileHandles, KeGetCurrentThread(),
        HANDLE_FLAG_CLOSE_FILE_ON_EXIT, HANDLE_FILE, File, NULL
    );
    if (!FileHandle) SET_SOD_MEDIA_MANAGEMENT;

    File->Handle = FileHandle;

    return File;
}
KERNELSTATUS KERNELAPI DefaultCloseFile(FILE File) {
    File->Handle->Flags &= ~(HANDLE_FLAG_CLOSE_FILE_ON_EXIT); // File is manually closed, calling CloseHandle will cause CloseFile and causes stack overflow, ... problems
    CloseHandle(File->Handle);
    RemoteFreePool(kproc, File->Path);
    SZeroMemory(File);
    return KERNEL_SOK;
}


int FsMountDevice(RFDEVICE_OBJECT device){
    if(!ValidateDevice(device)) return -1;
    void* sect0 = AllocatePoolEx(kproc, 0x1000, 0, 0);
    if(!sect0) SET_SOD_OUT_OF_RESOURCES;
    memset(sect0,0, 0x1000);


    if(KERNEL_ERROR(FsReadDrive((DISK_DEVICE_INSTANCE*)device->ExtensionPointer,0,1,sect0))) {
        RemoteFreePool(kproc, sect0);
        return -1;
    }

    if(KERNEL_ERROR(MountMbrDevice((DISK_DEVICE_INSTANCE*)device->ExtensionPointer, sect0))) {
        RemoteFreePool(kproc, sect0);
        return -2;
    }
    RemoteFreePool(kproc, sect0);
    return SUCCESS;
}

