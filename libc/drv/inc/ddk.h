// DRIVER DEVELOPEMENT KIT
#pragma once





#ifdef __KERNEL
#include <krnltypes.h>
#else

#define DDKIMPORT __declspec(dllimport)
#define __exSDRIVER__ // Executable System Driver

#define OSAPI __stdcall
#define DDKAPI OSAPI

#define DBGV (UINT64) // Cast Value to UINT64 For System Debug Print

#include <kerneltypes.h>
#include <sysipc.h>
#include <ipcserver.h>
#include <pciexpressapi.h>
#include <dci.h>
#include <stdlib.h>


#define DDKENTRY __cdecl



#define DDK_SUCCESS 0
#define DDK_ERROR -1

typedef long long DDKSTATUS;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *RFDRIVER_OBJECT;
typedef void* RFDEVICE_OBJECT;

typedef struct _DRIVER_IDENTIFICATION_DATA{
    BOOL Present; // Used to allocate (register) a driver without wasting space
    BOOL Enabled;
    UINT16 DriverPath[0x100];
    UINT8 DriverPathLength;
    UINT8 Reserved0[3];
    UINT DriverType;
    UINT LoadType; // Reserved
    UINT DeviceSearchType; // 0 = by class and subclass, 1 = by device_id
    UINT DataUnmask;
    UINT DeviceClass;
    UINT DeviceSubclass;
    UINT ProgramInterface;
    UINT64 VendorId;
    UINT64 DeviceId;
    UINT DeviceSource;
    UINT Reserved1;
} DRIVER_IDENTIFICATION_DATA;



typedef struct _GUID{
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} GUID;

typedef struct _DRIVER_AUTHENTICATION_TABLE{
    BOOL Status;
    GUID CompanyGuid;
    UINT32 GlobalType;
    UINT32 MajorDriverVersion;
    UINT32 MinorDriverVersion;
    UINT16 DriverName[50];
    UINT16 DriverNameEndCharCode; // Set to 0
    UINT16 NameCount;
    UINT16 NameRvaPointers[]; // Relative Virtual Address of names
} DRIVER_AUTHENTICATION_TABLE;



#define GET_IRQNUMBER(InterruptNumber) (InterruptNumber - 0x20)

#endif

#define MAX_DRIVER_DEVICES 0x200

typedef long long(__cdecl* DRIVER_STARTUP_PROCEDURE)(RFDRIVER_OBJECT DriverObject);
typedef long long(__cdecl* DRIVER_UNLOAD_PROCEDURE)(RFDRIVER_OBJECT DriverObject);


typedef struct _DRIVER_OBJECT {
    UINT Status; // 1 present, 2 enabled (3), 4 Loaded
    UINT64 DriverId;
    RFPROCESS Process;
    UINT MajorKernelVersion;
    UINT MinorKernelVersion;
    UINT MajorOsVersion;
    UINT MinorOsVersion;
    DRIVER_IDENTIFICATION_DATA DriverIdentificationCopy;
    // Fields specified with 
    UINT NumDevices;
    RFDEVICE_OBJECT Devices[MAX_DRIVER_DEVICES];
    DRIVER_STARTUP_PROCEDURE DriverStartup;
    DRIVER_UNLOAD_PROCEDURE DriverUnload;
} DRIVER, *RFDRIVER_OBJECT;

#ifndef __KERNEL
typedef enum _DEVICE_FEATURE{
    DEVICE_64BIT_ADDRESS_ALLOCATIONS = 1,
    DEVICE_FORCE_MEMORY_ALLOCATION = 2 // OS Will Terminate the device if an allocation failure occurs
} DEVICE_FEATURE;
#ifndef __DLL_EXPORTS

DDKSTATUS DDKENTRY DriverEntry(RFDRIVER_OBJECT DriverObject);
DDKSTATUS DDKENTRY UnloadDriver(RFDRIVER_OBJECT DriverObject);

// DDKIMPORT KERNELSTATUS DDKAPI SystemDebugPrintA(const char* Text);

DDKIMPORT PVOID DDKAPI VirtualAllocZero(unsigned long long Size);

// Unimplemented for now

DDKIMPORT UINT64 DDKAPI PciDeviceConfigurationRead64(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
DDKIMPORT UINT32 DDKAPI PciDeviceConfigurationRead32(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
DDKIMPORT UINT16 DDKAPI PciDeviceConfigurationRead16(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
DDKIMPORT UINT8 DDKAPI PciDeviceConfigurationRead8(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);

DDKIMPORT void* DDKAPI PciGetBaseAddress(RFDEVICE_OBJECT DeviceObject, unsigned char BarIndex, BOOL* MmIO /*0 = IO, 1 = MMIO*/);
DDKIMPORT UINT64 DDKAPI IoRead64(RFDEVICE_OBJECT DeviceObject, UINT64 Offset);
DDKIMPORT UINT32 DDKAPI IoRead32(RFDEVICE_OBJECT DeviceObject, UINT64 Offset);
DDKIMPORT UINT16 DDKAPI IoRead16(RFDEVICE_OBJECT DeviceObject, UINT64 Offset);
DDKIMPORT UINT8 DDKAPI IoRead8(RFDEVICE_OBJECT DeviceObject, UINT64 Offset);

DDKIMPORT UINT32 DDKAPI IoPciRead32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
DDKIMPORT UINT16 DDKAPI IoPciRead16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
DDKIMPORT UINT8 DDKAPI IoPciRead8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);

#define ALIGN_VALUE(Value, Align) ((Value & (Align - 1)) ? ((Value + Align) & ~(Align - 1)) : Value)


#define BUFFERWRITE32(Buffer, Value) (*(UINT32*)&Buffer = Value)
#define BUFFERREAD32(Buffer) (*(UINT32*)&Buffer)
DDKIMPORT BOOL DDKAPI WriteDeviceLog(RFDEVICE_OBJECT DeviceObject, LPCWSTR EventString);
DDKIMPORT BOOL DDKAPI WriteDeviceLogA(RFDEVICE_OBJECT DeviceObject, LPCSTR EventString);

// Primarly used to declare a bad device configuration, if device is fine. Call to this function is not necessary
DDKIMPORT BOOL DDKAPI SetDeviceStatus(RFDEVICE_OBJECT DeviceObject, KERNELSTATUS DeviceStatus);

// Set a pointer to a structure used by device driver
DDKIMPORT BOOL DDKAPI SetDeviceExtension(RFDEVICE_OBJECT DeviceObject, void* ExtensionPointer);
DDKIMPORT void* DDKAPI GetDeviceExtension(RFDEVICE_OBJECT DeviceObject);
DDKIMPORT BOOL DDKAPI PciEnableInterrupts(RFDEVICE_OBJECT Device);

// Gets interrupt pin, and request relative irq number from the kernel
DDKIMPORT UINT DDKAPI PciGetInterruptNumber(RFDEVICE_OBJECT Device);



DDKIMPORT BOOL DDKAPI SetDeviceFeature(RFDEVICE_OBJECT Device, UINT64 DeviceFeatureMask);

// The allocated memory is automatically initialized to 0, if DeviceFeatures.64BitAllocations, the returned address may be > 4GB
DDKIMPORT void* DDKAPI AllocateDeviceMemory(RFDEVICE_OBJECT Device, UINT64 NumBytes, UINT Alignment);

DDKIMPORT void DDKAPI PciDeviceConfigurationWrite64(RFDEVICE_OBJECT DeviceObject, UINT64 Value, UINT16 Offset);
DDKIMPORT void DDKAPI PciDeviceConfigurationWrite32(RFDEVICE_OBJECT DeviceObject, UINT32 Value, UINT16 Offset);
DDKIMPORT void DDKAPI PciDeviceConfigurationWrite16(RFDEVICE_OBJECT DeviceObject, UINT16 Value, UINT16 Offset);
DDKIMPORT void DDKAPI PciDeviceConfigurationWrite8(RFDEVICE_OBJECT DeviceObject, UINT8 Value, UINT16 Offset);


DDKIMPORT void DDKAPI IoPciWrite32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT32 Value, UINT16 Offset);
DDKIMPORT void DDKAPI IoPciWrite16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Value, UINT16 Offset);
DDKIMPORT void DDKAPI IoPciWrite8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT8 Value, UINT16 Offset);




#pragma section(".DRVAUTH", read)
static __declspec(allocate(".DRVAUTH")) DRIVER_AUTHENTICATION_TABLE GlobalAuthenticationDescriptor;

#else
#define DDKEXPORT __declspec(dllexport)

#define NATIVE_IDT_IST 3
#define KERNEL_CS 0x08
DDKEXPORT UINT32 DDKAPI IoPciRead32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
DDKEXPORT UINT16 DDKAPI IoPciRead16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
DDKEXPORT UINT8 DDKAPI IoPciRead8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);

DDKEXPORT void DDKAPI IoPciWrite32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT32 Value, UINT16 Offset);
DDKEXPORT void DDKAPI IoPciWrite16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Value, UINT16 Offset);
DDKEXPORT void DDKAPI IoPciWrite8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT8 Value, UINT16 Offset);


typedef struct __DEVICE_OBJECT _DEVICE_OBJECT, *_RFDEVICE_OBJECT;

DDKEXPORT UINT32 DDKAPI PciDeviceConfigurationRead32(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
DDKEXPORT UINT16 DDKAPI PciDeviceConfigurationRead16(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
DDKEXPORT UINT8 DDKAPI PciDeviceConfigurationRead8(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset);

DDKEXPORT void DDKAPI PciDeviceConfigurationWrite64(_RFDEVICE_OBJECT DeviceObject, UINT64 Value, UINT16 Offset);
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite32(_RFDEVICE_OBJECT DeviceObject, UINT32 Value, UINT16 Offset);
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite16(_RFDEVICE_OBJECT DeviceObject, UINT16 Value, UINT16 Offset);
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite8(_RFDEVICE_OBJECT DeviceObject, UINT8 Value, UINT16 Offset);

#endif



#endif
#ifndef __KERNEL
enum PRIVILEGE_LEVELS{
    PRIVILEGE_KERNEL = 0, // Driver interrupt gate must be in kernel privilege
    PRIVILEGE_USER = 3
};

enum IDT_DESCRIPTOR_TYPE{
    IDT_CALL_GATE = 12,
    IDT_INTERRUPT_GATE = 14,
    IDT_TRAP_GATE = 15
};

#endif

