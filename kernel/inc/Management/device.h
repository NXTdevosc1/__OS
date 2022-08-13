#pragma once
#include <krnltypes.h>
#include <CPU/process.h>
#include <Management/handle.h>
#include <Management/lock.h>

typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *RFDEVICE_OBJECT;
typedef struct _DEVICE_LIST DEVICE_LIST, * PDEVICE_LIST;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *RFDRIVER_OBJECT;

#define DEVICE_LEGACY_IO_FLAG ((UINT64)1 << 63)
#define DEVICE_BASE_ADDRESS_INDEX_MAX 6
#define MAX_DEVICE_INDEX 100
// #define MAKE_IOADDR(IoPort) (LPVOID)((UINT64)IoPort & 0xFFFFFFFF | DEVICE_LEGACY_IO_FLAG)

enum DEVICE_TYPE {
	DEVICE_TYPE_UNKNOWN = 0,
	DEVICE_TYPE_STORAGE = 1,
	DEVICE_TYPE_USB_CONTROLLER = 2,
	DEVICE_TYPE_TIMER = 3
};

typedef struct _DEVICE_OBJECT {
	BOOL Present;
	UINT64 Id;
	RFDEVICE_OBJECT ParentDevice;
	BOOL DeviceMutex; // Mutex
	BOOL DeviceControl; // Set if the device driver is controlling the device
	BOOL DeviceDisable;
	BOOL AccessMethod; // 0 for Legacy Io Access, 1 For MMIO Access
	LPWSTR DisplayName;
	UINT32 DeviceSource;
	UINT32 DeviceClass;
	UINT32 DeviceSubclass;
	UINT32 ProgramInterface;
	UINT32 DeviceType;
	UINT64 VendorId;
	UINT64 DeviceId;
	LPVOID DeviceConfiguration; // For ex. In PCI, this is the mmio address of the header
	UINT8 Bus; // In case that Access Method is 0 (Legacy IO Access)
	UINT8 DeviceNumber;
	UINT8 Function;
	UINT8 PciAccessType; // 0 = MMIO, 1 = IO
	RFDRIVER_OBJECT Driver;
	void* ControlInterface;
	void* ExtensionPointer; // an optionnal structure for additionnal info about the device
	BOOL DeviceInitialized; // set to 1 when the relative driver completes the device initialization, the device then becomes accessible
	UINT64 DeviceFeatures;
	KERNELSTATUS DeviceStatus;
} DEVICE_OBJECT, * RFDEVICE_OBJECT;



typedef struct _DEVICE_LIST {
	UINT64 Index;
	DEVICE_OBJECT Devices[MAX_DEVICE_INDEX];
	PDEVICE_LIST Next;
} DEVICE_LIST, * PDEVICE_LIST;

typedef struct _DEVICE_ITERATOR {
	UINT64 DeviceIndex;
	BOOL Present;
	BOOL Finish;
	PDEVICE_LIST List;
} DEVICE_ITERATOR, *PDEVICE_ITERATOR;

typedef struct _DEVICE_MANAGEMENT_TABLE {
	UINT64 NumDevices;
	DEVICE_LIST DeviceList;
} DEVICE_MANAGEMENT_TABLE;

#define IOPCI_CONFIGURATION(Bus, Device, Function) ((void*)(Function | (Device << 8) | (Bus << 16) | DEVICE_LEGACY_IO_FLAG))

typedef enum _DEVICE_CLASS {
    DEVICE_CLASS_ATA_DRIVE = 0x100,
    DEVICE_CLASS_USB_DRIVE = 0x101
} DEVICE_CLASS;

RFDEVICE_OBJECT KEXPORT KERNELAPI InstallDevice(
RFDEVICE_OBJECT ParentDevice,
UINT32 DeviceSource,
UINT32 DeviceType,
UINT32 DeviceClass,
UINT32 DeviceSubclass,
UINT32 VendorId,
LPVOID DeviceConfigurationPtr // Used in PCI Configuration Space (if PCI Source then Class, Subclass are not required to be set)
); // this is mostly done by kernel, note : device instances cannot be unregistred
										   // Unregistring can only be done by setting device disable, the device can only be re-enabled by control process
RFDRIVER_OBJECT KERNELAPI FindDeviceDriver(RFDEVICE_OBJECT Device);

BOOL KERNELAPI ControlDevice(void* DriverObject, RFDEVICE_OBJECT Device);
BOOL KERNELAPI ReleaseDeviceControl(RFDEVICE_OBJECT Device);

BOOL KERNELAPI DisableDevice(RFDEVICE_OBJECT Device);
BOOL KERNELAPI EnableDevice(RFDEVICE_OBJECT Device);


BOOL KERNELAPI LockDeviceAccess(RFDEVICE_OBJECT Device);
BOOL KERNELAPI UnlockDeviceAccess(RFDEVICE_OBJECT Device);
LPVOID KERNELAPI QueryDeviceInterface(RFDEVICE_OBJECT Device);
BOOL KERNELAPI SetDeviceInterface(RFDEVICE_OBJECT Device, void* DeviceInterface);

BOOL KEXPORT KERNELAPI SetDeviceDisplayName(RFDEVICE_OBJECT Device, LPWSTR DeviceName);

BOOL KERNELAPI StartDeviceIterator(PDEVICE_ITERATOR Iterator);
RFDEVICE_OBJECT KERNELAPI GetNextDevice(PDEVICE_ITERATOR Iterator);
BOOL KERNELAPI EndDeviceIterator(PDEVICE_ITERATOR Iterator);
BOOL KERNELAPI ValidateDevice(RFDEVICE_OBJECT Device);

extern DEVICE_MANAGEMENT_TABLE DeviceManagementTable;