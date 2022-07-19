#include <fs/fat32/read.h>
#include <MemoryManagement.h>
#include <preos_renderer.h>
#include <fs/fat32/fat32.h>
#include <cstr.h>
#include <preos_renderer.h>

int FAT32_FsGetCluster(uint32_t* fat, uint32_t index){
    if((fat[index] & FAT32_CLUSTER_MASK) == FAT32_END_OF_CHAIN) return -2;
    if((fat[index] & FAT32_CLUSTER_MASK) < 2) return -1;
    return (fat[index] & FAT32_CLUSTER_MASK);
}
KERNELSTATUS KERNELAPI FAT32_FsRead(FILE File, UINT64 Offset, UINT64* NumBytesRead, void* Buffer){
    if (!File || !Buffer) return KERNEL_SERR_INVALID_PARAMETER;
    FILE_INFORMATION_STRUCT FileInformation = { 0 };
    if (KERNEL_ERROR(GetFileInfo(File, &FileInformation, NULL))) return KERNEL_SERR_INVALID_PARAMETER;
    
    UINT64 ReadSize = FileInformation.FileSize;
    
    if (NumBytesRead) {
        if (*NumBytesRead > ReadSize) *NumBytesRead = ReadSize;
        ReadSize = *NumBytesRead;
    }

    if (!ReadSize) return KERNEL_SOK;

    FAT_READ_STREAM* Stream = FatOpenReadStream(FileInformation.Cluster, ReadSize, Buffer, File->Partition);


    while (FatReadNextCluster(Stream, NULL)); // And done easilly

    FatCloseReadStream(Stream);
    

    return KERNEL_SOK;
}
