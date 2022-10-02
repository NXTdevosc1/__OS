// OS OPTIONS, SETTINGS, VARIABLES AND BCD File
#ifndef _NOINC
#include <krnltypes.h>
#include <Management/handle.h>
#endif
#define SYSTEM_SETFILE_SIGNATURE "$SETFILE"
#define SYSTEM_SETFILE_MAGIC0 0xF2CAFEE0BABE1F3A
#define SYSTEM_SETFILE_DESCRIPTION L"Settings Variables BCD File"
#define LEN_SETFILE_DESCRIPTION 0x1C
#define SETFILE_MAX_DIR_ENTRIES 0x100
#define SETFILE_MAX_PRIMARY_POINTERS 0x80
#define SETFILE_ENTRY_NAME_MAXLEN 0x80
#define SETFILE_ENTRY_VALUE_MAXLEN 0x100

#define SYSTEM_SETFILE_MAJOR_VERSION 1
#define SYSTEM_SETFILE_MINOR_VERSION 0

// GET SYSTEM KEY DESCRIPTOR TYPE
#define SYSTEM_ENTRY_ATTRIBUTES 0
#define SYSTEM_ENTRY_NAME 1
#define SYSTEM_ENTRY_NUM_FILES 2



typedef struct _SYSTEM_SETFILE_HEADER SYSTEM_SETFILE_HEADER;
typedef struct _SYSTEM_SETFILE_ENTRY SYSTEM_SETFILE_ENTRY;

typedef enum _SYSTEM_SETFILE_ATTRIBUTES{
    SETFILE_PRESENT = 1,
    SETFILE_DIRECTORY = 2,
    SETFILE_VIRTUAL = 4, // Used on the virtual filesystem
    SETFILE_PRIVILEGED_USER_ACCESS = 8,
    SETFILE_KEY_DIRECTORY = 0x10,
    SETFILE_KEY_READONLY = 0x20
} SYSTEM_SETFILE_ATTRIBUTES;

typedef struct _SYSTEM_SETFILE_ENTRY{
    UINT64 Attributes;
    UCHAR  EntryName[SETFILE_ENTRY_NAME_MAXLEN];
    UCHAR EON; // End Of Name
    UINT64 DirPointer; // if Attributes.Directory then this is the pointer of the directory start
    UINT64 RuntimePointer;
    UCHAR KeyValue[SETFILE_ENTRY_VALUE_MAXLEN]; // if Attributes.Directory this is directory description, otherwise this is the value stored on the entry
} SYSTEM_SETFILE_ENTRY;

#define SETFILE_CLUSTERS_PER_TABLE 0x400

typedef struct _FREE_CLUSTER_TABLE{
    UINT64 NumPresent; // if set to CLUSTERS_PER_TABLE then go to the next table
    struct {
        UINT64 Present : 1;
        UINT64 Offset : 63;
    } Clusters[SETFILE_CLUSTERS_PER_TABLE];
    UINT64 NextOffset;
} FREE_CLUSTER_TABLE;

typedef struct _SYSTEM_SETFILE_HEADER{
    UCHAR Signature[8]; // $SETFILE
    QWORD Magic0;
    UINT16 Description[LEN_SETFILE_DESCRIPTION];
    UINT16 MajorVersion; // Must be equal to SYSTEM_SETFILE_MAJOR_VERSION
    UINT16 MinorVersion; // Must be equal to SYSTEM_SETFILE_MINOR_VERSION
    UINT64 ExtensionPointer; // Extension pointer to support higher version
    UINT64 RuntimeExtensionPointer;
    UINT64 SystemPrimaryPointers[SETFILE_MAX_PRIMARY_POINTERS]; /*Used as pointers to */
    UINT64 RuntimeSystemPrimaryPointers[SETFILE_MAX_PRIMARY_POINTERS];
    FREE_CLUSTER_TABLE FreeClusterTable;
    SYSTEM_SETFILE_ENTRY RootDirectory[SETFILE_MAX_DIR_ENTRIES];
} SYSTEM_SETFILE_HEADER, *SETFILE;



KERNELSTATUS KERNELAPI SystemSetfile(void); // Initialize System Setfile


KERNELSTATUS KERNELAPI SystemWriteEntry(UCHAR* Path, UINT16 NumBytes, void* Buffer);
KERNELSTATUS KERNELAPI SystemReadEntry(UCHAR* Path, UINT16 NumBytes, void* Buffer);
KERNELSTATUS KERNELAPI SystemCreateEntry(UCHAR* Path /*-1 Means root directory*/, UCHAR* KeyName, UINT32 Attributes, UINT16 NumBytes, void* Buffer);
KERNELSTATUS KERNELAPI SystemDeleteEntry(UCHAR* Path);
KERNELSTATUS KERNELAPI SystemGetDescriptor(UCHAR* Path, UINT DescriptorType, UINT CheckNumBytes, void* Buffer);
KERNELSTATUS KERNELAPI SystemGetFile(UCHAR* Directory, UINT FileIndex, UCHAR* FilePath);
SETFILE gSystemSetfile;