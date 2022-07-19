#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#define KERNELAPI __cdecl
typedef long long KERNELSTATUS;
typedef unsigned long long QWORD;
#include <setfile.h>

#define OPEN_FILE_CHECK(argc, argv, MinArgc) if(argc < MinArgc) THROW("Please provide a File Name and a Path"); UCHAR* Path = argv[3]; HANDLE File = CreateFileA(argv[2], GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);if(File == INVALID_HANDLE_VALUE) THROW("Failed to open the specified file");SYSTEM_SETFILE_HEADER* Header = malloc(sizeof(SYSTEM_SETFILE_HEADER));if(!Header) ALLOCATION_ERROR;if(!ReadFile(File, Header, sizeof(SYSTEM_SETFILE_HEADER), NULL, NULL))THROW("Failed to read the specified file");if(!SetfileCheckHeader(Header)) THROW("Invalid or corrupted SETFILE");

#define THROW(err) {printf(err); return -1;}
#define ALLOCATION_ERROR {printf("Memory allocation failed. Exiting..."); return -2;}
char* HelpCommandList[] = {
    "help : Get a list of possible arguments",
    "setfile [File Name] : Creates a SET_FILE",
    "desc [EntryPath] : Get a descriptor of this entry",
    "create [EntryPath] [Attributes ... (dir, privileged, keydir)]: Creates an entry on the specified file path",
    "delete [EntryPath] : Deletes the specified file",
    "setdesc [EntryPath] [DescType] [DescriptorValue]: Modifies Descriptor of a specified entry"
    "list [DirPath] : List the specified directory (if no dir specified, it lists the root dir)"
};

int StringCompare(char* x, char* y);
int SetFile(int argc, char** argv);
int Create(int argc, char** argv);
int List(int argc, char** argv);
int Desc(int argc, char** argv);

SYSTEM_SETFILE_ENTRY* FindEntry(SYSTEM_SETFILE_HEADER* Header, UCHAR* Path, HANDLE File){
    BOOL Found = FALSE;
    UINT Index = 0;
    UCHAR FileName[SETFILE_ENTRY_NAME_MAXLEN] = {0};
    UINT16 LenFileName = 0;
    OVERLAPPED Overlapped = {0};
    for(;;){
        LenFileName = 0;
        if(!*Path) return NULL;

        while(*Path != '/' && *Path != '\\' && *Path != 0){
            FileName[LenFileName] = *Path;
            Path++;
            LenFileName++;
        }
        while(*Path == '/' || *Path == '\\'){
            Path++; // for the /, \, or the \0
        }
        FileName[LenFileName] = 0;

        for(UINT i = 0;i<SETFILE_MAX_DIR_ENTRIES;i++){
            if(Header->RootDirectory[i].Attributes & SETFILE_PRESENT &&
            StringCompare(Header->RootDirectory[i].EntryName, FileName)
            ){
                if(*Path){
                if(Header->RootDirectory[i].Attributes & SETFILE_DIRECTORY){
                    Overlapped.Offset = Header->RootDirectory[i].DirPointer;
                    Overlapped.OffsetHigh = Header->RootDirectory[i].DirPointer >> 32;
                    ReadFile(File, Header->RootDirectory, sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES, NULL, &Overlapped);
                    Found = TRUE;
                    break;
                }else {
                    printf("%s Is not a directory", FileName);
                    return NULL;
                }
                }else {
                    return &Header->RootDirectory[i];
                }
            }
        }
        if(!Found) return NULL;
        Found = FALSE;
    }

    return NULL;
}

int Help(int argc, char** argv){
    printf("\nSystem Setfile Utility : Help\n\n");
    for(UINT i = 0;i<sizeof(HelpCommandList) / 8;i++){
        printf("- %s\n", HelpCommandList[i]);
    }
    return 0;
}
int SetFile(int argc, char** argv){
    if(argc <= 2) THROW("No Name provided.");

    SYSTEM_SETFILE_HEADER* Header = malloc(sizeof(SYSTEM_SETFILE_HEADER));
    if(!Header) ALLOCATION_ERROR;
    ZeroMemory(Header, sizeof(SYSTEM_SETFILE_HEADER));
    memcpy(Header->Signature, SYSTEM_SETFILE_SIGNATURE, 8);
    memcpy(Header->Description, SYSTEM_SETFILE_DESCRIPTION, LEN_SETFILE_DESCRIPTION << 1);
    Header->Magic0 = SYSTEM_SETFILE_MAGIC0;
    Header->MajorVersion = SYSTEM_SETFILE_MAJOR_VERSION;
    Header->MinorVersion = SYSTEM_SETFILE_MINOR_VERSION;

    HANDLE File = CreateFileA(argv[2], GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(!File) THROW("Failed to create the specified file.");

    if(!WriteFile(File, Header, sizeof(SYSTEM_SETFILE_HEADER), NULL, NULL))
        THROW("Failed to write to specified file");


    CloseHandle(File);
    return TRUE;
}

inline BOOL SetfileCheckHeader(SYSTEM_SETFILE_HEADER* Header){
    if(memcmp(Header->Signature, SYSTEM_SETFILE_SIGNATURE, 8) != 0 ||
    memcmp(Header->Description, SYSTEM_SETFILE_DESCRIPTION, LEN_SETFILE_DESCRIPTION << 1) != 0 ||
    Header->Magic0 != SYSTEM_SETFILE_MAGIC0
    ) return FALSE;

    
    return TRUE;
}

// Create Entry
int Create(int argc, char** argv){
    OPEN_FILE_CHECK(argc, argv, 4);

    UINT len = strlen(argv[3]);
    if(!len || len > SETFILE_ENTRY_NAME_MAXLEN) THROW("Too long file path.");

    UCHAR FileName[SETFILE_ENTRY_NAME_MAXLEN] = {0};
    UINT16 LenFileName = 0;
    BOOL isFileName = FALSE;
    OVERLAPPED Overlapped = {0};

    UINT64 Offset = (UINT64)Header->RootDirectory - (UINT64)Header;

    for(;;){
        LenFileName = 0;
        while(*Path != '/' && *Path != '\\'){
            FileName[LenFileName] = *Path;
            if(!*Path) {
                isFileName = TRUE;
                break;
            }
            Path++;
            LenFileName++;
        }
        Path++; // for the /, \, or the \0
        FileName[LenFileName] = 0;
        if(isFileName){
            printf("File Name : %s\n", FileName);
            for(UINT i = 0;i<SETFILE_MAX_DIR_ENTRIES;i++){
                if(!(Header->RootDirectory[i].Attributes & SETFILE_PRESENT)){
                    memset(&Header->RootDirectory[i], 0, sizeof(SYSTEM_SETFILE_ENTRY));
                    memcpy(Header->RootDirectory[i].EntryName, FileName, LenFileName);
                    Header->RootDirectory[i].Attributes = SETFILE_PRESENT;
                    UINT NumAttributes = argc - 4;
                    printf("Num Attributes : %d\n", NumAttributes);
                    for(UINT c = 0;c<NumAttributes;c++){
                        if(StringCompare(argv[4 + c], "dir")){
                            Header->RootDirectory[i].Attributes |= SETFILE_DIRECTORY;
                        }else if(StringCompare(argv[4 + c], "privileged")){
                            Header->RootDirectory[i].Attributes |= SETFILE_PRIVILEGED_USER_ACCESS;
                        }else if(StringCompare(argv[4 + c], "keydir")){
                            Header->RootDirectory[i].Attributes |= SETFILE_DIRECTORY;
                            Header->RootDirectory[i].Attributes |= SETFILE_KEY_DIRECTORY;
                        }else if(StringCompare(argv[4 + c], "readonly")){
                            Header->RootDirectory[i].Attributes |= SETFILE_KEY_READONLY;
                        }else {
                            printf("Ignoring Unknown Attribute : %s\n", argv[4 + c]);
                        }
                    }
                    
                    Offset+=i*sizeof(SYSTEM_SETFILE_ENTRY);
                    
                    printf("Offset to write : %I64x", Offset);
                    
                    

                    if(Header->RootDirectory[i].Attributes & SETFILE_DIRECTORY){
                        UINT64 DirPointer = 0;
                        // Allocate Cluster
                        UINT64 ClusterTableOffset = (UINT64)&Header->FreeClusterTable - (UINT64)&Header;

                        for(;;){
                            for(UINT x = 0;x<SETFILE_CLUSTERS_PER_TABLE;x++){
                                if(Header->FreeClusterTable.Clusters[x].Present){
                                    DirPointer = Header->FreeClusterTable.Clusters[x].Offset;
                                    Header->FreeClusterTable.Clusters[x].Present = 0;
                                    break;
                                }
                            }
                            if(DirPointer || !Header->FreeClusterTable.NextOffset) break;
                            ClusterTableOffset = Header->FreeClusterTable.NextOffset;
                            Overlapped.Offset = ClusterTableOffset;
                            Overlapped.OffsetHigh = ClusterTableOffset >> 32;
                            if(!ReadFile(File, &Header->FreeClusterTable, sizeof(FREE_CLUSTER_TABLE), NULL, &Overlapped))
                                THROW("Failed to read the specified file");
                        }
                        DWORD SzHigh = 0;
                        if(!DirPointer) DirPointer = GetFileSize(File, &SzHigh);
                        DirPointer |= (UINT64)SzHigh >> 32;

                        Header->RootDirectory[i].DirPointer = DirPointer;
                        printf("Dir Pointer : %I64x", Header->RootDirectory[i].DirPointer);
                        Overlapped.Offset = Offset;
                        Overlapped.OffsetHigh = Offset >> 32;
                        if(!WriteFile(File, &Header->RootDirectory[i], sizeof(SYSTEM_SETFILE_ENTRY), NULL, &Overlapped))
                            THROW("Failed to write the specified file.");

                        SYSTEM_SETFILE_ENTRY* bf = malloc(sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES);
                        if(!bf) ALLOCATION_ERROR;

                        ZeroMemory(bf, sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES);
                        Overlapped.Offset = DirPointer;
                        Overlapped.OffsetHigh = DirPointer >> 32;
                        if(!WriteFile(File, bf, sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES, NULL, &Overlapped))
                            THROW("Failed to write the specified file.");

                        return 0;
                    }

                    
                    return 0;
                }
            }
            THROW("Failed to allocate file, Directory is Full.");
        }else{
            printf("Path Entry Name : %s\n", FileName);
            BOOL Found = FALSE;
            for(UINT i = 0;i<SETFILE_MAX_DIR_ENTRIES;i++){
                if(Header->RootDirectory[i].Attributes & SETFILE_PRESENT &&
                StringCompare(Header->RootDirectory[i].EntryName, FileName)
                ){
                    if(Header->RootDirectory[i].Attributes & SETFILE_DIRECTORY){
                        Overlapped.Offset = Header->RootDirectory[i].DirPointer;
                        Overlapped.OffsetHigh = Header->RootDirectory[i].DirPointer >> 32;
                        if(!ReadFile(File, &Header->RootDirectory, sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES, NULL, &Overlapped))
                            THROW("Failed to read the specified file");

                        Offset = Header->RootDirectory[i].DirPointer;
                        Found = TRUE;
                        break;
                    }else {
                        printf("%s Is not a directory", FileName);
                        return -1;
                    }
                }
            }
            if(!Found) THROW("Cannot find the directory specified");
        }
    }

    return 0;
}

int List(int argc, char** argv){
    OPEN_FILE_CHECK(argc, argv, 4);

    UINT len = strlen(argv[3]);
    if(!len || len > SETFILE_ENTRY_NAME_MAXLEN) THROW("Too long file path.");

    UCHAR FileName[SETFILE_ENTRY_NAME_MAXLEN] = {0};
    UINT16 LenFileName = 0;

    
    BOOL DirFound = FALSE;
    OVERLAPPED Overlapped = {0};
    BOOL Found = FALSE;
    char text[0x200] = {0};


    for(;;){
        LenFileName = 0;
        while(*Path == '/' || *Path == '\\'){
                Path++; // for the /, \, or the \0
        }
        if(!*Path) DirFound = TRUE;
        else{
            while(*Path != '/' && *Path != '\\' && *Path != 0){
                FileName[LenFileName] = *Path;
                Path++;
                LenFileName++;
            }
            FileName[LenFileName] = 0;
        }
        if(DirFound){
            if(!FileName[0]) FileName[0] = '/';
            printf("Current Directory : %s, Path : %s\n\n", FileName, argv[3]);
            for(UINT i = 0;i<SETFILE_MAX_DIR_ENTRIES;i++){
                if(Header->RootDirectory[i].Attributes & SETFILE_PRESENT){
                    sprintf_s(text, 0x200, "File : %s       ", Header->RootDirectory[i].EntryName);
                    if(Header->RootDirectory[i].Attributes & SETFILE_DIRECTORY){
                        sprintf_s(text, 0x200, "%s Directory    ", text);
                    }
                    if(Header->RootDirectory[i].Attributes & SETFILE_KEY_DIRECTORY){
                        sprintf_s(text, 0x200, "%s Key Directory    ", text);
                    }
                    if(Header->RootDirectory[i].Attributes & SETFILE_KEY_READONLY){
                        sprintf_s(text, 0x200, "%s Read Only    ", text);
                    }
                    if(Header->RootDirectory[i].Attributes & SETFILE_PRIVILEGED_USER_ACCESS){
                        sprintf_s(text, 0x200, "%s Privileged User Access   ", text);
                    }

                    printf("%s\n", text);
                }
            }
            return 0;
        }else{
            for(UINT i = 0;i<SETFILE_MAX_DIR_ENTRIES;i++){
                if(Header->RootDirectory[i].Attributes & SETFILE_PRESENT &&
                StringCompare(Header->RootDirectory[i].EntryName, FileName)
                ){
                    if(!(Header->RootDirectory[i].Attributes & SETFILE_DIRECTORY)) {
                        printf("%s Is not a directory", Header->RootDirectory[i].EntryName);
                        return -1;
                    }else{
                        Overlapped.Offset = Header->RootDirectory[i].DirPointer;
                        Overlapped.OffsetHigh = Header->RootDirectory[i].DirPointer >> 32;
                        if(!ReadFile(File, &Header->RootDirectory, sizeof(SYSTEM_SETFILE_ENTRY) * SETFILE_MAX_DIR_ENTRIES, NULL, &Overlapped))
                            THROW("Failed to read the specified file");

                        Found = TRUE;
                        break;
                    }
                }
            }
            if(!Found) THROW("Cannot find the specified directory");
            Found = FALSE;
        }
    }
}
int Desc(int argc, char** argv){
    OPEN_FILE_CHECK(argc, argv, 4);
    SYSTEM_SETFILE_ENTRY* Entry = FindEntry(Header, Path, File);
    if(!Entry){
        THROW("The entry does not exist.");
    }else{
        char text[0x200] = {0};
        sprintf_s(text, 0x200, "File : %s       ", Entry->EntryName);
        if(Entry->Attributes & SETFILE_DIRECTORY){
            sprintf_s(text, 0x200, "%s Directory    ", text);
        }
        if(Entry->Attributes & SETFILE_KEY_DIRECTORY){
            sprintf_s(text, 0x200, "%s Key Directory    ", text);
        }
        if(Entry->Attributes & SETFILE_KEY_READONLY){
            sprintf_s(text, 0x200, "%s Read Only    ", text);
        }
        if(Entry->Attributes & SETFILE_PRIVILEGED_USER_ACCESS){
            sprintf_s(text, 0x200, "%s Privileged User Access   ", text);
        }

        printf("%s", text);
    }
    return 0;
}

#define NUM_COMMANDS 5

struct {
    char* CommandName;
    int (__cdecl* Function)(int argc, char** argv);
} Commands[NUM_COMMANDS] = {
    {"help", Help},
    {"setfile", SetFile},
    {"create", Create},
    {"list", List},
    {"desc", Desc}
};

int StringCompare(char* x, char* y){
    for(;;){
        if(*x == 0 && *y == 0) return 1;

        if(*x != *y) return 0;
        x++;
        y++;
    }
    return 0;
}

int main(int argc, char** argv){
    if(argc <= 1) THROW("No arguments received. type setf help to get a list of possible commands.");
    for(UINT i = 0;i<NUM_COMMANDS;i++){
        if(StringCompare(argv[1], Commands[i].CommandName)){
            return Commands[i].Function(argc, argv);
        }
    }
    THROW("Unknown Command, type setf help to get a list of possible commands");
    return -1;
}