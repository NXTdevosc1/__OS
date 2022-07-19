#pragma once
#include <krnltypes.h>
#include <dsk/disk.h>
// SUFFIX 'i' Means Interface


KERNELSTATUS KERNELAPI FAT32i_RawPartitionRead(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 NumClusters, void* Buffer);
KERNELSTATUS KERNELAPI FAT32i_RawPartitionWrite(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 NumClusters, void* Buffer);

