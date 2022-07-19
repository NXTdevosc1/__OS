#pragma once
#include <fs/fs.h>
#include <fs/fat32/fat32.h>

wchar_t* FAT32ParseShortFileNameEntry(struct FAT_DIRECTORY_ENTRY* entry, wchar_t* out, UINT64* FileNameLength);
wchar_t* FAT32ParseLongFileNameEntry(struct FAT_LFN_DIR_ENTRY* lfne, struct FAT_DIRECTORY_ENTRY** next_entry, int* err, wchar_t* out, UINT64* FileNameLength);