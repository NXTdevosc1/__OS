#include <Management/device.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <sys/drv.h>
#include <CPU/cpu.h>
#include <IO/pcie.h>
#include <interrupt_manager/SOD.h>
#include <IO/pci.h>
DEVICE_MANAGEMENT_TABLE DeviceManagementTable = { 0 };

RFDRIVER_OBJECT KERNELAPI FindDeviceDriver(RFDEVICE_OBJECT Device){
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	for(UINT64 i = 0;i<DriverTable->TotalDrivers;i++){
		if(DriverTable->Drivers[i].Present && DriverTable->Drivers[i].Enabled &&
		DriverTable->Drivers[i].DeviceSource == Device->DeviceSource
		) {
			DRIVER_IDENTIFICATION_DATA* DriverIdentification = &DriverTable->Drivers[i];
			if(DriverIdentification->DeviceSearchType == DEVICE_SEARCH_BYCLASS){
				if(Device->DeviceClass != DriverIdentification->DeviceClass) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_SUBCLASS && Device->DeviceSubclass != DriverIdentification->DeviceSubclass) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_PROGRAM_INTERFACE && Device->ProgramInterface != DriverIdentification->ProgramInterface) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_VENDOR_ID && Device->VendorId != DriverIdentification->VendorId) continue;
			}else if(DriverIdentification->DeviceSearchType == DEVICE_SEARCH_BYID){
				if(Device->DeviceId != DriverIdentification->DeviceId || Device->VendorId != DriverIdentification->VendorId) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_CLASS && DriverIdentification->DeviceClass != Device->DeviceClass) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_SUBCLASS && DriverIdentification->DeviceSubclass != Device->DeviceSubclass) continue;
				if(DriverIdentification->DataUnmask & DATA_UNMASK_PROGRAM_INTERFACE && DriverIdentification->ProgramInterface != Device->ProgramInterface) continue;
			}

			// If everything is right then return the appropriate driver object or create it if it does not exists
			RFDRIVER_OBJECT DriverObject = QueryDriverById(i);
			if(!DriverObject) {
				DriverObject = LoadDriverObject(i);
				if(!DriverObject) SOD(SOD_DEVICE_MANAGEMENT, "FAILED TO LOAD DRIVER OBJECT (DATA CORRUPTION EXPECTED)");
			}
			return DriverObject;
		}
	}
	return NULL;
}

RFDEVICE_OBJECT KERNELAPI InstallDevice(RFDEVICE_OBJECT ParentDevice, UINT32 DeviceSource, LPVOID DeviceConfiguration) // this is mostly done by kernel, note : device instances cannot be unregistred
{
	if (!DeviceConfiguration) return NULL;
	if (ParentDevice && !ValidateDevice(ParentDevice)) return NULL;
	PDEVICE_LIST list = &DeviceManagementTable.DeviceList;
	for (UINT64 ListIndex = 0;;ListIndex++) {
		for(UINT i = 0;i<MAX_DEVICE_INDEX;i++){
			if(list->Devices[i].Present) continue;
			RFDEVICE_OBJECT Device = &list->Devices[i];
			SZeroMemory(Device);
			Device->Present = 1;
			Device->DeviceSource = DeviceSource;
			Device->ParentDevice = ParentDevice;
			Device->DeviceConfiguration = DeviceConfiguration;


			Device->AccessMethod = 1; // MMIO Access


			if(DeviceSource == DEVICE_SOURCE_PCI){
				if((UINT64)DeviceConfiguration & DEVICE_LEGACY_IO_FLAG) {
					// Get Bus, Device, and function (DEVICE_LEGACY_IO_FLAG Automatically truncated)
					Device->Bus = (UINT64)DeviceConfiguration >> 16;
					Device->DeviceNumber = (UINT64)DeviceConfiguration >> 8;
					Device->Function = (UINT64)DeviceConfiguration;

					Device->DeviceClass = IoPciRead8(Device->Bus, Device->DeviceNumber, Device->Function, PCI_CLASS);
					Device->DeviceSubclass = IoPciRead8(Device->Bus, Device->DeviceNumber, Device->Function, PCI_SUBCLASS);
					Device->ProgramInterface = IoPciRead8(Device->Bus, Device->DeviceNumber, Device->Function, PCI_PROGRAM_INTERFACE);
					Device->VendorId = IoPciRead16(Device->Bus, Device->DeviceNumber, Device->Function, PCI_VENDOR_ID);
					Device->DeviceId = IoPciRead16(Device->Bus, Device->DeviceNumber, Device->Function, PCI_DEVICE_ID);
					Device->PciAccessType = 1; // IO Access

					SystemDebugPrint(L"LEGACY PCI DEVICE : (%d,%d,%d) CLASS = %x, SUBCLASS = %x, PROGIF = %x", (UINT64)Device->Bus, (UINT64)Device->DeviceNumber, (UINT64)Device->Function, (UINT64)Device->DeviceClass, (UINT64)Device->DeviceSubclass, (UINT64)Device->ProgramInterface);

				}else{
					PCI_CONFIGURATION_HEADER* PciConfig = Device->DeviceConfiguration;
					UINT64 _RawConfig = (UINT64)Device->DeviceConfiguration >> 12;
					Device->Function = _RawConfig & 0b111;
					Device->DeviceNumber = (_RawConfig >> 3) & 0b11111;
					Device->Bus = _RawConfig >> 8;

					Device->DeviceClass = PciConfig->DeviceClass;
					Device->DeviceSubclass = PciConfig->DeviceSubclass;
					Device->ProgramInterface = PciConfig->ProgramInterface;
					Device->VendorId = PciConfig->VendorId;
					Device->DeviceId = PciConfig->DeviceId;

					SystemDebugPrint(L"PCI Express DEVICE : (%d,%d,%d) CLASS = %x, SUBCLASS = %x, PROGIF = %x", (UINT64)Device->Bus, (UINT64)Device->DeviceNumber, (UINT64)Device->Function, (UINT64)Device->DeviceClass, (UINT64)Device->DeviceSubclass, (UINT64)Device->ProgramInterface);

				}

			}

			RFDRIVER_OBJECT Driver = FindDeviceDriver(Device);
			
			if(Driver) {
				if(!ControlDevice(Driver, Device)) SOD(SOD_DEVICE_MANAGEMENT, "FAILED TO CONTROL DEVICE");
				SystemDebugPrint(L"Driver#%d Controls Device %x", Driver->DriverId, Device);
			}
			DeviceManagementTable.NumDevices++;
			return Device;
		}
		if (!list->Next) {
			list->Next = kmalloc(sizeof(DEVICE_LIST));
			list = list->Next;
			SZeroMemory(list);
		}
		else list = list->Next;
	}
}


BOOL KERNELAPI ControlDevice(RFDRIVER_OBJECT DriverObject, RFDEVICE_OBJECT Device){
	__SpinLockSyncBitTestAndSet(&Device->DeviceMutex, 0);
	if(DriverObject->NumDevices == MAX_DRIVER_DEVICES || Device->DeviceControl) {
		__BitRelease(&Device->DeviceMutex, 0);
		return FALSE; // Max devices acheived
	}
	for(UINT i = 0;i<MAX_DRIVER_DEVICES;i++){
		if(!DriverObject->Devices[i]) {
			DriverObject->Devices[i] = Device;
			break;
		}
	}
	DriverObject->NumDevices++;

	__BitRelease(&Device->DeviceMutex, 0);
	return TRUE;
}

BOOL KERNELAPI ValidateDevice(RFDEVICE_OBJECT Device) {
	if (!Device || !Device->Present) return FALSE;
	UINT64 DeviceListIndex = Device->Id / MAX_DEVICE_INDEX;
	UINT64 DeviceIndex = Device->Id % MAX_DEVICE_INDEX;
	PDEVICE_LIST list = &DeviceManagementTable.DeviceList;
	for (UINT64 i = 0; i < DeviceListIndex; i++) {
		if (!list->Next) return FALSE;
		list = list->Next;
	}
	if (&list->Devices[DeviceIndex] == Device) return TRUE;
	return FALSE;
}

BOOL KERNELAPI DisableDevice(RFDEVICE_OBJECT Device) {
	if (!Device) return FALSE;
	if (Device->DeviceDisable) return TRUE;
	RFPROCESS Process = KeGetCurrentProcess();

	// if (!Device->DeviceControl || Device->ControlDriverProcess != Process) return FALSE;
	// Device->DeviceDisable = TRUE;
	return TRUE;
}
BOOL KERNELAPI EnableDevice(RFDEVICE_OBJECT Device) {
	if (!Device) return FALSE;
	if (!Device->DeviceDisable) return TRUE;
	RFPROCESS Process = KeGetCurrentProcess();

	// if (!Device->DeviceControl || Device->ControlDriverProcess != Process) return FALSE;
	// Device->DeviceDisable = FALSE;
	return TRUE;
}

LPVOID KERNELAPI QueryDeviceInterface(RFDEVICE_OBJECT Device) {
	if (!Device || !ValidateDevice(Device)) return NULL;
	return Device->ControlInterface;
}

BOOL KERNELAPI SetDeviceInterface(RFDEVICE_OBJECT Device, void* DeviceInterface) {
	// Must have control of the device

	if (!Device || !ValidateDevice(Device) || !DeviceInterface) return FALSE;
	RFPROCESS Process = KeGetCurrentProcess();
	if (!Device->DeviceControl || Device->Driver->Process != Process) return FALSE;
	Device->ControlInterface = DeviceInterface;
	return TRUE;
}


BOOL KERNELAPI SetDeviceDisplayName(RFDEVICE_OBJECT Device, LPWSTR DeviceName) // Not that device name must be a fixed memory
{
	if (!Device || !ValidateDevice(Device)) return FALSE;
	RFPROCESS Process = KeGetCurrentProcess();
	if (!Device->DeviceControl || Device->Driver->Process != Process) return FALSE;
	Device->DisplayName = DeviceName; // If name is NULL Then original name is used
	return TRUE;
}

BOOL KERNELAPI StartDeviceIterator(PDEVICE_ITERATOR Iterator) {
	if (!Iterator || Iterator->Present) return FALSE;
	SZeroMemory(Iterator);
	Iterator->Present = TRUE;
	Iterator->List = &DeviceManagementTable.DeviceList;
	return TRUE;
}
BOOL KERNELAPI EndDeviceIterator(PDEVICE_ITERATOR Iterator) {
	if (!Iterator || !Iterator->Present) return FALSE;
	SZeroMemory(Iterator);
	return TRUE;
}
RFDEVICE_OBJECT KERNELAPI GetNextDevice(PDEVICE_ITERATOR Iterator) {
	if (!Iterator || !Iterator->Present || Iterator->Finish) return NULL;
	
	for (;;) {
		
		if (Iterator->List->Index > Iterator->DeviceIndex) {
			UINT64 Index = Iterator->DeviceIndex;
			Iterator->DeviceIndex++;
			return &Iterator->List->Devices[Index];
		}

		if (!Iterator->List->Next) {
			Iterator->Finish = TRUE;
			break;
		}
		Iterator->DeviceIndex = 0;
		Iterator->List = Iterator->List->Next;
	}
	return NULL;
}
