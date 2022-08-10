#include <sys/drv.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <setfile.h>
#include <Management/runtimesymbols.h>
#include <interrupt_manager/SOD.h>
#include <kernel.h>
#include <sys/bootconfig.h>
#include <stdlib.h>
#include <loaders/pe64.h>

DRIVER_OBJECT* DriverObjects = NULL;

void RunEssentialExtensionDrivers(){
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	DriverObjects = ExtendedMemoryAlloc(NULL, sizeof(DRIVER_OBJECT) * DriverTable->TotalDrivers, 0x1000, NULL, 0);
	if(!DriverObjects) SET_SOD_MEMORY_MANAGEMENT;

	ZeroMemory(DriverObjects, sizeof(DRIVER_OBJECT) * DriverTable->TotalDrivers);



    FILE_IMPORT_ENTRY* FileImportEntry = (FILE_IMPORT_ENTRY*)FileImportTable;
	while(FileImportEntry->Type != FILE_IMPORT_ENDOFTABLE){
		if(FileImportEntry->Type == FILE_IMPORT_DRIVER){
			for(UINT i = 0;i<DriverTable->TotalDrivers;i++){
				if(DriverTable->Drivers[i].Present && DriverTable->Drivers[i].Enabled &&
				DriverTable->Drivers[i].DriverType == KERNEL_EXTENSION_DRIVER &&
				wstrlen(FileImportEntry->Path) == DriverTable->Drivers[i].DriverPathLength &&
				wstrcmp_nocs(FileImportEntry->Path, DriverTable->Drivers[i].DriverPath, DriverTable->Drivers[i].DriverPathLength)
				) {
					RFDRIVER_OBJECT DriverObject = LoadDriverObject(i);
					if(!DriverObject) SET_SOD_INITIALIZATION;

					RunDriver(DriverObject);
					break;
				}
			}
		}
		FileImportEntry++;
	}
}

void RunDeviceDrivers(){
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	for(UINT i = 0;i<DriverTable->TotalDrivers;i++){
		if(DriverObjects[i].Status & 1 /*must not be loaded, not running*/ && DriverObjects[i].DriverIdentificationCopy.DriverType == DEVICE_DRIVER) {
			if(KERNEL_ERROR(RunDriver(&DriverObjects[i]))) SET_SOD_PROCESS_MANAGEMENT;
		}
	}
}

void LoadExtensionDrivers(){
    RFDRIVER_TABLE DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
    for(UINT64 i = 0;i<DriverTable->TotalDrivers;i++){
        if(DriverTable->Drivers[i].DriverType == KERNEL_EXTENSION_DRIVER){
            // Load the driver
        }
    }
}

RFDRIVER_OBJECT KERNELAPI LoadDriverObject(UINT64 DriverIdentificationId){
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	if(DriverIdentificationId >= DriverTable->TotalDrivers) return NULL;
	DRIVER_IDENTIFICATION_DATA* DriverIdentification = &DriverTable->Drivers[DriverIdentificationId];
	if(!DriverIdentification->Present || !DriverIdentification->Enabled) return NULL;
	RFDRIVER_OBJECT Driver = &DriverObjects[DriverIdentificationId];
	if(Driver->Status & DRIVER_STATUS_PRESENT) return Driver;
	Driver->Status |= DRIVER_STATUS_PRESENT;
	memcpy(&Driver->DriverIdentificationCopy, DriverIdentification, sizeof(DRIVER_IDENTIFICATION_DATA));
	Driver->DriverId = DriverIdentificationId;
	Driver->MajorKernelVersion = MAJOR_KERNEL_VERSION;
	Driver->MinorKernelVersion = MAJOR_KERNEL_VERSION;

	BOOT_CONFIGURATION* BootConfiguration = FileImportTable[FIMPORT_BOOT_CONFIG].LoadedFileBuffer;

	Driver->MajorOsVersion = BootConfiguration->Header.MajorOsVersion;
	Driver->MinorOsVersion = BootConfiguration->Header.MinorOsVersion;
	Driver->Process = CreateProcess(NULL, DriverIdentification->DriverPath, SUBSYSTEM_NATIVE, KERNELMODE_PROCESS);
	if(!Driver->Process) SET_SOD_PROCESS_MANAGEMENT;

	return Driver;
}

RFDRIVER_OBJECT KERNELAPI QueryDriverById(UINT64 DriverId){
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	if(DriverId >= DriverTable->TotalDrivers) return NULL;
	if(DriverObjects[DriverId].Status & 1) return &DriverObjects[DriverId];

	return NULL;
}

BOOL KERNELAPI isDriver(RFDRIVER_OBJECT DriverObject){
	if(!DriverObject || GetPhysAddr(kproc, DriverObject) != DriverObject) return FALSE;
	DRIVER_TABLE* DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	if(DriverObject->DriverId >= DriverTable->TotalDrivers ||
	&DriverObjects[DriverObject->DriverId] != DriverObject
	) return FALSE;

	return TRUE;
}

KERNELSTATUS KERNELAPI RunDriver(RFDRIVER_OBJECT DriverObject){
	// Check if its an imported driver
	if(DriverObject->Status & (DRIVER_STATUS_RUNNING | DRIVER_STATUS_LOADED)) return KERNEL_SERR;
	void* Buffer = NULL;
	for(UINT i = 0;FileImportTable[i].Type != FILE_IMPORT_ENDOFTABLE;i++){
		if(FileImportTable[i].Type == FILE_IMPORT_DRIVER ||
		FileImportTable[i].Type == FILE_IMPORT_DEVICE_DRIVER ||
		FileImportTable[i].Type == FILE_SYSTEM_DRIVER)
		{
			if(!wstrcmp_nocs(FileImportTable[i].Path, DriverObject->DriverIdentificationCopy.DriverPath, DriverObject->DriverIdentificationCopy.DriverPathLength + 1)) continue;
			Buffer = FileImportTable[i].LoadedFileBuffer;
		}
	}
	if(Buffer){
		if(KERNEL_ERROR(Pe64LoadNativeApplication(Buffer, DriverObject))) return KERNEL_SERR;
		RFTHREAD Thread = CreateThread(DriverObject->Process, DriverObject->StackSize, DriverObject->DriverStartup, THREAD_CREATE_SUSPEND, NULL);
		if(!Thread) SET_SOD_PROCESS_MANAGEMENT;
		SetThreadPriority(Thread, THREAD_PRIORITY_ABOVE_NORMAL);
		Thread->Registers.rcx = (UINT64)DriverObject;
		__STACK_PUSH(Thread->Registers.rsp, Thread->Registers.rcx);
		ResumeThread(Thread);
	}
	
	// otherwise, use file system & disk drivers to load the driver
	return KERNEL_SOK;
}