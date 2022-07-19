#include <fs/fat32/interface.h>
#include <fs/fat32/fat32.h>
KERNELSTATUS KERNELAPI FAT32i_RawPartitionRead(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 NumClusters, void* Buffer) {
	if (ClusterOffset < 2) return KERNEL_SERR_INVALID_PARAMETER;
	ClusterOffset -= 2;
	struct FAT32_FS_DESCRIPTOR* FsDescriptor = Partition->FsInfo;
	return Partition->Interface.SectorRawRead(Partition, Partition->PartitionInformation.BaseAddress + FsDescriptor->reserved_area_size + FsDescriptor->fat_count * FsDescriptor->fat_size +  ClusterOffset * Partition->PartitionInformation.SectorsInCluster, NumClusters * Partition->PartitionInformation.SectorsInCluster, Buffer);
}
KERNELSTATUS KERNELAPI FAT32i_RawPartitionWrite(PARTITION_INSTANCE* Partition, UINT64 ClusterOffset, UINT64 NumClusters, void* Buffer) {
	if (ClusterOffset < 2) return KERNEL_SERR_INVALID_PARAMETER;

	ClusterOffset -= 2;
	struct FAT32_FS_DESCRIPTOR* FsDescriptor = Partition->FsInfo;
	return Partition->Interface.SectorRawWrite(Partition, Partition->PartitionInformation.BaseAddress + FsDescriptor->reserved_area_size + FsDescriptor->fat_count * FsDescriptor->fat_size + ClusterOffset * Partition->PartitionInformation.SectorsInCluster, NumClusters * Partition->PartitionInformation.SectorsInCluster, Buffer);
}