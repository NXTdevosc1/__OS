#pragma once
#include <CPU/process.h>
#include <krnltypes.h>
#include <dsk/disk.h>



BOOL MSABI FsCreateDirList(RFPROCESS Process, DIRECTORY_FILE_LIST* List);
int MSABI FsReleaseDirList(RFPROCESS process, DIRECTORY_FILE_LIST* dirls);
int MSABI FsDirNext(DIRECTORY_FILE_LIST* list, DIR_LIST_FILE_ENTRY** out);
int MSABI FsDirPrevious(DIRECTORY_FILE_LIST list, DIRECTORY_FILE_LIST* out);