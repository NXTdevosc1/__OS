#pragma once
#include <krnltypes.h>
#include <Management/device.h>
#include <CPU/process_defs.h>

#define DRVTBL_SIG L"Ke_$DRVTBL"
typedef struct _DRIVER_OBJECT DRIVER, *RFDRIVER_OBJECT;

#pragma pack(push, 1)

typedef struct _DRIVER_IDENTIFICATION_DATA DRIVER_IDENTIFICATION_DATA;

#define DRIVER_STATUS_PRESENT 1
#define DRIVER_STATUS_RUNNING 2
#define DRIVER_STATUS_LOADED 4

typedef enum DRIVER_TYPE{
    INVALID_DRIVER_TYPE = 0,
    KERNEL_EXTENSION_DRIVER = 1,
    DEVICE_DRIVER = 2,
    FILE_SYSTEM_DRIVER = 3,
    MAX_DRIVER_TYPE = 3
} DRIVER_TYPE;

// Specifies additionnal data to be used on device search
typedef enum _DRIVER_DATA_UNMASK{
    DATA_UNMASK_SUBCLASS = 1, // used on both
    DATA_UNMASK_VENDOR_ID = 2, // used only on class/subclass search
    DATA_UNMASK_CLASS = 4, // used only on search type by id
    DATA_UNMASK_PROGRAM_INTERFACE = 8 // used on both
} DRIVER_DATA_UNMASK;

typedef enum _DRIVER_DEVICE_SOURCE{
    DEVICE_SOURCE_PCI = 0, // Load device driver from both PCI & PCIE
    DEVICE_SOURCE_USB = 1,
    DEVICE_SOURCE_ACPI = 2,
    DEVICE_SOURCE_SYSTEM_RESERVED = 3, // Driven by kernel
    DEVICE_SOURCE_PARENT_CONTROLLER = 4
} DRIVER_DEVICE_SOURCE;

typedef enum _DEVICE_SEARCH_TYPE{
    DEVICE_SEARCH_BYCLASS = 0,
    DEVICE_SEARCH_BYID = 1
} DEVICE_SEARCH_TYPE;

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

typedef struct _DRIVER_TABLE{
    // Driver Table Header
    UINT16 Signature[10];
    UINT64 NumKernelExtensionDrivers;
    UINT64 NumThirdPartyDrivers;
    UINT64 TotalDrivers; // Must be = NumKexDrivers + NumThirdPartyDrivers
    UINT16 HeaderEnd[10]; // = KEHEADER_END

    DRIVER_IDENTIFICATION_DATA Drivers[];
} DRIVER_TABLE, *RFDRIVER_TABLE;

#pragma pack(pop)

typedef long long(__cdecl* DRIVER_STARTUP_PROCEDURE)(RFDRIVER_OBJECT DriverObject);
typedef long long(__cdecl* DRIVER_UNLOAD_PROCEDURE)(RFDRIVER_OBJECT DriverObject);

#define MAX_DRIVER_DEVICES 0x200

typedef struct _DRIVER_OBJECT {
    UINT Status; // 1 present, 2 Running (3), 4 Loaded
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
    UINT64 StackSize;
} DRIVER, *RFDRIVER_OBJECT;

// Register driver into system
KERNELSTATUS KERNELAPI InstallDriver(DRIVER_IDENTIFICATION_DATA* DriverIdentification);

// Create a runnable driver object
RFDRIVER_OBJECT KERNELAPI LoadDriverObject(UINT64 DriverIdentificationId);
KERNELSTATUS KERNELAPI RunDriver(RFDRIVER_OBJECT DriverObject);

BOOL KERNELAPI isDriver(RFDRIVER_OBJECT DriverObject);

// Driver id is the index in the Driver Identification Table
RFDRIVER_OBJECT KERNELAPI QueryDriverById(UINT64 DriverId);

void RunEssentialExtensionDrivers(void);
void LoadExtensionDrivers(void);
void RunDeviceDrivers(void);