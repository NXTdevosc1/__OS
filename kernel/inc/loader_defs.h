#pragma once
#include <Uefi.h>
#include <krnltypes.h>
// #ifndef ___EDK2___
// #include <efi.h>
// #include <loaders/pe.h>
// #else
// // EDK2 Official UEFI Specification & Implementation
// #include <Uefi.h>
// #endif
typedef struct _OPTIONNAL_HEADER_DATA_DIRECTORIES *RFOPTIONNAL_HEADER_DATA_DIRECTORIES;


#define SECTION_NAME_SIZE 8
#define _PRT_VRFY ".PRTVRFY"
#define _INITDATA "INITDATA"
#define _FIMPORT  "FIMPORT"

#pragma pack(push, 1)


struct PSF1_HEADER{
	UINT8 magic[2];
	UINT8 mode;
	UINT8 character_size;
};

enum FILE_IMPORT_TYPE{
	FILE_IMPORT_ENDOFTABLE,
	FILE_IMPORT_DRIVER,
	FILE_IMPORT_DLL,
	FILE_IMPORT_DATA,
	FILE_IMPORT_SETFILE,
	FILE_IMPORT_DEVICE_DRIVER,
	FILE_IMPORT_FILE_SYSTEM_DRIVER
};


typedef struct _FILE_IMPORT_ENTRY{
	unsigned int Type;
	unsigned long long LoadedFileSize;
	void* LoadedFileBuffer;
	unsigned short* BaseName; // e.g. Path=System/dll.dll BaseName=dll.dll
	unsigned short LenBaseName;
	unsigned short Path[150]; // Path inside system
} FILE_IMPORT_ENTRY;



struct PSF1_FONT{
	struct PSF1_HEADER header;
	char  glyph_buffer[];
};
typedef struct _FRAME_BUFFER_DESCRIPTOR FRAME_BUFFER_DESCRIPTOR;
typedef void (__cdecl *FRAME_BUFFER_UPDATE_VIDEO)
(FRAME_BUFFER_DESCRIPTOR* FrameBufferDescriptor, unsigned int x, unsigned int y,
unsigned int Width, unsigned int Height
);

typedef struct _FRAME_BUFFER_DESCRIPTOR FRAME_BUFFER_DESCRIPTOR;


typedef struct _FRAME_BUFFER_DESCRIPTOR{
	unsigned int		FrameBufferId; // For multiple gpus/displays
	unsigned int 		HorizontalResolution;
	unsigned int 		VerticalResolution;
	unsigned int 		Version;
	char* 			FrameBufferBase; // for G.O.P / VESA VBE
	unsigned long long 	FrameBufferSize;
	unsigned int 		ColorDepth;
	unsigned int 		RefreshRate;
	void*				Protocol; // Pointer used by UpdateVideo routine
	unsigned long long Pitch;
	FRAME_BUFFER_DESCRIPTOR* Next;
} FRAME_BUFFER_DESCRIPTOR;

typedef struct _MEMORY_DESCRIPTOR {
	unsigned char Type;
	unsigned long long PageCount;
	unsigned long long PhysicalStart;
} MEMORY_DESCRIPTOR;

typedef struct _MEMORY_MAP{
	unsigned long long Count;
	MEMORY_DESCRIPTOR MemoryDescriptors[];
} MEMORY_MAP;


typedef struct _KERNEL_INITIALISATION_DATA {
	MEMORY_MAP* MemoryMap;
	UINT64 NumFrameBuffers; // Number of display Frame Buffers
	FRAME_BUFFER_DESCRIPTOR* fb;
	struct PSF1_FONT* start_font;
	int uefi;
	unsigned long long NumConfigurationTables;
	EFI_CONFIGURATION_TABLE* SystemConfigurationTables;
	EFI_RUNTIME_SERVICES* RuntimeServices;
	void* ImageBase;
	UINT64 ImageSize;
	int	Reserved1;
	EFI_SYSTEM_TABLE* EfiSystemTable;
	RFOPTIONNAL_HEADER_DATA_DIRECTORIES PEDataDirectories; // Used for e.g. to export kernel functions to drivers
} INITDATA;

#pragma pack(pop)
