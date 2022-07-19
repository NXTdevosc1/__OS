#pragma once
#include <stdint.h>
#include <fs/fs.h>
int FAT32_FsGetCluster(uint32_t* fat, uint32_t index); // a value of -1 means invalid cluster, -2 means last cluster
// returns read size in int
KERNELSTATUS KERNELAPI FAT32_FsRead(FILE File, UINT64 Offset, UINT64* NumBytesRead, void* Buffer);