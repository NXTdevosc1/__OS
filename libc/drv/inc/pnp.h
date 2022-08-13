#include <krnlapi.h>
#pragma once

// Used in InstallDevice in case that Device source = DEVICE_SOURCE_PCI
#define IOPCI_CONFIGURATION(Bus, Device, Function) ((void*)(Function | (Device << 8) | (Bus << 16) | DEVICE_LEGACY_IO_FLAG))

enum DEVICE_TYPE {
	DEVICE_TYPE_UNKNOWN = 0,
	DEVICE_TYPE_STORAGE = 1,
	DEVICE_TYPE_USB_CONTROLLER = 2,
	DEVICE_TYPE_TIMER = 3
};

typedef enum _DRIVER_DEVICE_SOURCE{
    DEVICE_SOURCE_PCI = 0, // Load device driver from both PCI & PCIE
    DEVICE_SOURCE_USB = 1,
    DEVICE_SOURCE_ACPI = 2,
    DEVICE_SOURCE_SYSTEM_RESERVED = 3, // Driven by kernel (Do not set to this value)
    DEVICE_SOURCE_PARENT_CONTROLLER = 4
} DRIVER_DEVICE_SOURCE;

typedef enum _DEVICE_CLASS {
    DEVICE_CLASS_ATA_DRIVE = 0x100,
    DEVICE_CLASS_USB_DRIVE = 0x101
} DEVICE_CLASS;
typedef enum _DEVICE_SUBCLASS {
    DEVICE_SUBCLASS_AHCI_DRIVE = 0x100,
    DEVICE_SUBCLASS_IDE_DRIVE = 0x101
} DEVICE_SUBCLASS;

typedef void* RFDEVICE_OBJECT;

RFDEVICE_OBJECT KERNELAPI InstallDevice(
    RFDEVICE_OBJECT ParentDevice,
    UINT32 DeviceSource,
    UINT32 DeviceType,
    UINT32 DeviceClass,
    UINT32 DeviceSubclass,
    UINT32 VendorId,
    LPVOID DeviceConfigurationPtr // Used in PCI Configuration Space (if PCI Source then Class, Subclass are not required to be set)
); // this is mostly done by kernel, note : device instances cannot be unregistred

BOOL KERNELAPI SetDeviceDisplayName(RFDEVICE_OBJECT Device, LPWSTR DeviceName);