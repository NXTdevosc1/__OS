#pragma once

#ifndef BOOT
#include <krnltypes.h>
#endif // !BOOT


#define PE_IMAGE_SIGNATURE "PE\0\0"
#define PE_SECTION_LIMIT 96
#define PE_TARGET_MACHINE 0x8664

#define PE_SECTION_CODE 0x20
#define PE_SECTION_INITIALIZED_DATA 0x40
#define PE_SECTION_UNINITIALIZED_DATA 0x80

#define SECTION_IMPORT_DATA ".idata\0\0"
#define SECTION_EXPORT_DATA ".edata\0\0"

#define SIZEOF_OPTIONNAL_HEADER_DATA_DIRECTORIES 240
#define OPTIONNAL_NUM_DATA_DIRECTORIES 16

enum PE_CHARACTERISTICS {
    PE_RELOCS_STRIPPED = 1,
    PE_EXECUTABLE_IMAGE = 2,
    PE_LINE_NUMS_STRIPPED = 4,
    PE_LOCAL_SYMS_STRIPPED = 8,
    PE_AGGRESSIVE_WS_TRIM = 0x10,
    PE_LARGE_ADDRESS_AWARE = 0x20,
    PE_RESERVED0 = 0x40,
    PE_BYTES_RESERVED_LO = 0x80,
    PE_32BIT_MACHINE = 0x100,
    PE_DEBUG_STRIPPED = 0x200,
    PE_REMOVABLE_RUN_FROM_SWAP = 0x400,
    PE_NET_RUN_FROM_SWAP = 0x800,
    PE_FILE_SYSTEM = 0x1000,
    PE_FILE_DLL = 0x2000,
    PE_FILE_UP_SYSTEM_ONLY = 0x4000,
    PE_FILE_BYTES_RESERVED_HI = 0x8000
};
enum PE_SUBSYSTEM {
    PE_SUBSYSTEM_UNKNOWN = 0,
    PE_SUBSYSTEM_NATIVE = 1,
    PE_SUBSYSTEM_GUI = 2
};
enum IMAGE_DLL_CHARACTERISTICS {
    IMAGE_DLL_DYNAMIC_BASE = 0x40
};

typedef struct _IMAGE_IMPORT_ADDRESS_TABLE {
    UINT64 ImportAddress;
    UINT32 ForwarderRva;
} IMAGE_IMPORT_ADDRESS_TABLE, *PIMAGE_IMPORT_ADDRESS_TABLE;

// We support only PE32+ 64 bit image files
typedef struct {
    UINT16 magic; // must be 0x20b
    UINT8 MajorLinkerVersion;
    UINT8 MinorLinkerVersion;
    UINT32 SizeofCode;
    UINT32 SizeofInitializedData;
    UINT32 SizeofUninitializedData; // .bss ...
    UINT32 EntryPointAddr;
    UINT32 BaseOfCode;
} PE_OPTIONAL_HEADER;

typedef struct {
    UINT64 ImageBase;
    UINT32 SectionAlignment; // greater than or equal to file alignement
    UINT32 FileAlignment;
    UINT16 MajorOsVersion;
    UINT16 MinorOsVersion;
    UINT16 MajorImageVersion;
    UINT16 MinorImageVersion;
    UINT16 MajorSubsystemVersion;
    UINT16 MinorSubsystemVersion;
    UINT32 Win32VersionValue; // reserved must be 0
    UINT32 SizeofImage;
    UINT32 SizeofHeaders;
    UINT32 CheckSum;
    UINT16 Subsystem; // this is used to load image ex. from main or wWinMain or driver entry ...
    UINT16 DllCharacteristics;
    UINT64 StackReserve;
    UINT64 StackCommit;
    UINT64 HeapReserve;
    UINT64 HeapCommit;
    UINT32 LoaderFlags; // reserved must be 0
    UINT32 NumDataDirectories;
} PE_WINDOWS_SPECIFIC_FIELDS;
typedef struct {
    char name[8];
    UINT32 VirtualSize;
    UINT32 VirtualAddress;
    UINT32 SizeofRawData; // if less than virtual size reminder of file align is filled with 0
    UINT32 PtrToRawData; // for object file it should be 4 byte aligned for best performance
    UINT32 PtrToRelocations; // should be 0 for executables
    UINT32 PtrToLineNumbers;
    UINT16 NumRelocations; // set to 0 for executables
    UINT16 NumLineNumbers;
    UINT32 Characteristics; // section flags
} PE_SECTION_TABLE;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

typedef struct _OPTIONNAL_HEADER_DATA_DIRECTORIES {
    IMAGE_DATA_DIRECTORY ExportTable;
    IMAGE_DATA_DIRECTORY ImportTable;
    IMAGE_DATA_DIRECTORY ResourceTable;
    IMAGE_DATA_DIRECTORY ExceptionTable;
    IMAGE_DATA_DIRECTORY CertificateTable;
    IMAGE_DATA_DIRECTORY BaseRelocationTable;
    IMAGE_DATA_DIRECTORY Debug;
    IMAGE_DATA_DIRECTORY Architecture;
    IMAGE_DATA_DIRECTORY GlobalPtr;
    IMAGE_DATA_DIRECTORY ThreadLocalStorageTable;
    IMAGE_DATA_DIRECTORY LoadConfigTable;
    IMAGE_DATA_DIRECTORY BoundImport;
    IMAGE_DATA_DIRECTORY ImportAddressTable;
    IMAGE_DATA_DIRECTORY DelayImportDescriptor;
    IMAGE_DATA_DIRECTORY ClrRuntimeHeader;
    IMAGE_DATA_DIRECTORY Reserved;
} OPTIONNAL_HEADER_DATA_DIRECTORIES;

typedef struct {
    char Signature[4]; // PE\0\0
    UINT16 MachineType; // must be AMD64, for x86_64 and AMD64
    UINT16 NumSections;
    UINT32 TimeDateStamp;
    UINT32 SymbolTableOffset;
    UINT32 NumSymbols;
    UINT16 SizeofOptionnalHeader;
    UINT16 Characteristics;
    PE_OPTIONAL_HEADER OptionnalHeader; // not optional because it needs to be a loadable image
    PE_WINDOWS_SPECIFIC_FIELDS ThirdHeader;
    OPTIONNAL_HEADER_DATA_DIRECTORIES OptionnalDataDirectories;
} PE_IMAGE_HDR;



typedef struct _IMAGE_IMPORT_DIRECTORY {
    DWORD ImportLookupTable;
    DWORD TimeDataStamp;
    DWORD ForwarderChain;
    DWORD NameRva;
    DWORD ImportAddressTableRva;
} IMAGE_IMPORT_DIRECTORY, *PIMAGE_IMPORT_DIRECTORY;

enum IMAGE_BASE_RELOCATION_TYPE{
    IMAGE_REL_BASED_ABSOLUTE = 0,
    IMAGE_REL_BASED_HIGH = 1,
    IMAGE_REL_BASED_LOW = 2,
    IMAGE_REL_BASED_HIGHLOW = 3,
    IMAGE_REL_BASED_HIGHADJ = 4,
    IMAGE_REL_BASED_MIPS_JMPADDR = 5,
    IMAGE_REL_BASED_DIR64 = 10
};

typedef struct _IMAGE_BASE_RELOCATION_ENTRY {
    WORD Offset : 12; // 4 bits Type, 12 bits Offset
    WORD Type : 4;
} IMAGE_BASE_RELOCATION_ENTRY, * PIMAGE_BASE_RELOCATION_ENTRY;

typedef struct _IMAGE_BASE_RELOCATION_BLOCK {
    DWORD PageRva; // Page Relative Virtual Address
    DWORD BlockSize; // total num bytes in base relocation block
    IMAGE_BASE_RELOCATION_ENTRY RelocationEntries[];
} IMAGE_BASE_RELOCATION_BLOCK, *PIMAGE_BASE_RELOCATION_BLOCK;

typedef struct _IMAGE_EXPORT_DIRECTORY {
    UINT32 ExportFlags; // reserved must be 0
    UINT32 TimeDateStamp;
    UINT16 MajorVersion;
    UINT16 MinorVersion;
    UINT32 NameRva; // Dll Name
    UINT32 OrdinalBase;
    UINT32 NumAddressTableEntries;
    UINT32 NumNamePointers; // = NumOrdinalEntries
    UINT32 ExportAddressTableRva;
    UINT32 NamePointerRva;
    UINT32 OrdinalTableRva;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_EXPORT_ADDRESS_TABLE {
    UINT32 ExportRva;
    UINT32 ForwarderRva; // this one gives the dllname.exportname or export name with ordinal number
} IMAGE_EXPORT_ADDRESS_TABLE, *PIMAGE_EXPORT_ADDRESS_TABLE;

typedef struct _IMAGE_HINT_NAME_TABLE {
    WORD Hint;
    char Name[];
} IMAGE_HINT_NAME_TABLE, *PIMAGE_HINT_NAME_TABLE;

#define IMAGE_ORDINAL_IMPORT_FLAG 0x8000000000000000
#define IMAGE_ORDINAL_NUMBER(LookupEntry) (LookupEntry & 0xffff)
#define IMAGE_HINT_NAME_RVA(LookupEntry) (LookupEntry & (0x7fffffff));