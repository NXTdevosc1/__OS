#pragma once
#include <stdint.h>
#include <fs/fs.h>
#include <dsk/disk.h>


KERNELSTATUS KERNELAPI FAT32_FsListDirectory(FILE File, DIRECTORY_FILE_LIST* FileList, UINT64 MaxFileCount);