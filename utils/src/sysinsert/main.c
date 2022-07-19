#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#pragma pack(1)

enum SYSF_TYPES {
    SFT_SYSTEM_CONFIG = 0,
    SFT_MODULE = 1,
    SFT_DRIVER_LOADER = 2,
    SFT_SYS_REGISTRY = 3,
    SFT_USER = 4,
    SFT_BOOT_CONFIGURATION_DATA = 5,
    SFT_DRIVER = 7,
    SFT_SERVICE = 8,
    SFT_STARTUP_PROGRAM = 9
};
struct SYS_CONFIG_ENTRY {
    UINT8 type;
    UINT8 flags;
    LPWSTR path[];
};

struct SYS_FILE_HDR {
    UINT8 magic0[4];
    UINT16 osver_major;
    UINT16 osver_minor;
    UINT64 magic1;
    UINT8 type; // one of SYSF_TYPES
     //[num_paths] paths are 8 byte alligned
    // uint8_t type, uint8_t flags if bit 0 is set then its a folder, wchar_t paths[] starting from path_base_offset 
};
struct SYS_CONFIG_HDR {
    struct SYS_FILE_HDR hdr;
    UINT8 reserved;
    UINT32 num_paths;
    UINT16 checksum; // sizeof(struct SYS_FILE_HDR), typically 0x1C
    UINT32 paths_base;
    UINT32 path_offsets[];
};

char* SystemPath = "C:\\Users\\loukd\\Desktop\\OS\\kernel\\iso\\OS\\System\\";

int main(int argc, char** argv) {
    printf("Type system file name:\n");
    char FileName[100] = { 0 };
    if (!scanf("%s", FileName))
        return 1;
    printf("Opening %s ...\n", FileName);
    char FilePath[200] = { 0 };
    size_t SystemPathLength = strlen(SystemPath);
    size_t FileNameLength = strlen(FileName);
    memcpy(FilePath, SystemPath, SystemPathLength);
    memcpy((void*)(FilePath + SystemPathLength), FileName, FileNameLength);
    OFSTRUCT FileInfo = { 0 };
    HFILE File = OpenFile(FilePath, &FileInfo, OF_READWRITE);
    if (File == HFILE_ERROR) {
        printf("Failed to open %s\n", FileName);
        return 1;
    }
    printf("File successfully oppened.\nPath to insert:\n");
    WCHAR path[100] = { 0 };
    if (!scanf("%ls", path))
        return 1;

    int pathlen = lstrlenW(path);
    
    printf("Inserting Path : %ls\n", path);
    char* buffer = malloc(FileInfo.cBytes + 10 + 200);
    if (!buffer)
        return 2;
    OVERLAPPED overlapped = { 0 };
    if (!ReadFile(File, buffer, FileInfo.cBytes, NULL, &overlapped))
    {
        printf("Cannot read file.\n");
        return 1;
    }
    struct SYS_CONFIG_HDR* hdr = buffer;
    hdr->num_paths++;
    int cbytes = FileInfo.cBytes + (pathlen * 2);
    cbytes += 16 + (cbytes % 16);

    if (!WriteFile(File, buffer, FileInfo.cBytes, NULL, &overlapped))
    {
        printf("Failed to write file.\n");
        return 1;
    }

    

    printf("Path successfully inserted.\n");
    CloseHandle(File);
	return 0;
}