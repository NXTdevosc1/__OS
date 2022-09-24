#pragma once
#include <loaders/pe.h>
#include <krnltypes.h>
#include <CPU/process_defs.h>
#include <sys/drv.h>


int GetPEhdrOffset(void* hdr);
int PeCheckFileHeader(PE_IMAGE_HDR* hdr);
KERNELSTATUS Pe64LoadUserApplication(LPWSTR path);

KERNELSTATUS Pe64LoadNativeApplication(void* ImageBuffer, RFDRIVER_OBJECT DriverObject);
HRESULT Pe64ResolveImports(void* VirtualBuffer, void* ImageBuffer, PE_IMAGE_HDR* PeImage, RFPROCESS UserProcess);
PE_SECTION_TABLE* Pe64GetSectionByVirtualAddress(UINT32 VirtualAddress, PE_IMAGE_HDR* PeImage);
HRESULT Pe64LoadDll(void* ImageBuffer, void* VirtualBuffer, PE_IMAGE_HDR* ProgramImage, PIMAGE_IMPORT_DIRECTORY ImportDirectory, void** DllImage, UINT64* DllImageSize);
HRESULT Pe64RelocateImage(PE_IMAGE_HDR* PeImage, void* ImageBuffer, void* VirtualBuffer, void* ImageBase);
HRESULT Pe64LinkKernelSymbols(void* ProgramVirtualBuffer, PE_IMAGE_HDR* ProgramImage, PIMAGE_IMPORT_DIRECTORY ImportDirectory);