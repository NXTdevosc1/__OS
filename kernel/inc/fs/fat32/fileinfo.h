#pragma once
#include <krnltypes.h>
#include <dsk/disk.h>
#include <fs/fs.h>

KERNELSTATUS KERNELAPI FAT32_FsGetFileInfo(FILE File, FILE_INFORMATION_STRUCT* FileInformation, LPWSTR FileName);