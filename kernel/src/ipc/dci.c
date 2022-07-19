#include <ipc/dci.h>
#include <MemoryManagement.h>
#include <stdlib.h>
#include <CPU/process.h>
#include <loaders/subsystem.h>
#include <CPU/cpu.h>
// #include <interrupt_manager/SOD.h>
// DRIVER_LIST DriverList = { 0 };

// BOOL __ControlAllocateDriver = FALSE;
// RFDRIVER AllocateDriver() {
// 	__SpinLockSyncBitTestAndSet(&__ControlAllocateDriver, 0);
// 	DRIVER_LIST* List = &DriverList;
// 	for (UINT64 ListId = 0;;ListId++) {
// 		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
// 			if (!List->Drivers[i].Present) {
// 				List->Drivers[i].Present = TRUE;
// 				List->Drivers[i].DriverId = ListId * UNITS_PER_LIST + i;
// 				List->Drivers[i].ParentList = List;
// 				List->Drivers[i].InParentListIndex = i;
// 				__BitRelease(&__ControlAllocateDriver, 0);
// 				return &List->Drivers[i];
// 			}
// 		}
// 		if (!List->Next) {
// 			List->Next = kmalloc(sizeof(DRIVER_LIST));
// 			SZeroMemory(List->Next);
// 		}
// 		List = List->Next;
// 	}
// 	__BitRelease(&__ControlAllocateDriver, 0);
// 	return NULL;
// }

// BOOL isValidDriver(RFDRIVER Driver) {
// 	if (!Driver || !Driver->Present || !Driver->ParentList || Driver->InParentListIndex >= UNITS_PER_LIST
// 		|| &Driver->ParentList->Drivers[Driver->InParentListIndex] != Driver
// 		) return FALSE;
	
// 	return TRUE;
// }
// RFDRIVER CreateDriver(DRIVER_STARTUP_PROCEDURE EntryPoint, UINT64 DriverType, LPWSTR SystemPath, LPWSTR DriverName) {
// 	if (!EntryPoint || !SystemPath || !DriverName) return NULL;
// 	UINT64 LenSystemPath = wstrlen(SystemPath);
// 	UINT64 LenDriverName = wstrlen(DriverName);
// 	if (!LenSystemPath || !LenDriverName || LenSystemPath > MAX_DISPLAY_NAME_LENGTH || LenDriverName > MAX_DISPLAY_NAME_LENGTH)
// 		return NULL;


// 	RFDRIVER Driver = AllocateDriver();
// 	Driver->LenDriverName = LenDriverName;
// 	LPWSTR CopySystemPath = kmalloc((LenSystemPath + 1) << 1);
// 	LPWSTR CopyDriverName = kmalloc((LenDriverName + 1) << 1);
// 	CopySystemPath[LenSystemPath] = 0;
// 	CopyDriverName[LenDriverName] = 0;

// 	memcpy16(CopySystemPath, SystemPath, LenSystemPath);
// 	memcpy16(CopyDriverName, DriverName, LenDriverName);
// 	Driver->DriverType = DriverType;
// 	Driver->SystemPath = CopySystemPath;
// 	Driver->DriverName = CopyDriverName;
// 	Driver->DriverStartup = EntryPoint;

// 	// Setup Driver Tables
// 	Driver->Ports = kmalloc(sizeof(DCI_ACCESS_PORT));
// 	SZeroMemory(Driver->Ports);
// 	Driver->DeviceObjects = CreateHandleTable();

// 	return Driver;
// }
// BOOL LoadDriver(RFPROCESS Process, RFDRIVER Driver) {
// 	if (!isValidDriver(Driver) || Driver->Loaded) return FALSE;
// 	Driver->Process = Process;

// 	__STACK_PUSH(Driver->Process->StartupThread->Registers.rsp, 0);
// 	__STACK_PUSH(Driver->Process->StartupThread->Registers.rsp, Driver);

// 	Driver->Process->StartupThread->Registers.rcx = (UINT64)Driver; // Rcx is Arg0 Register for MSABI
// 	ThreadWrapperInit(Driver->Process->StartupThread, Driver->DriverStartup);
// 	ResumeThread(Driver->Process->StartupThread);

// 	return TRUE;
// }

// KERNELSTATUS UnloadDriver(RFDRIVER Driver){
// 	return KERNEL_SERR;
// }

// RFDRIVER OpenDriver(LPWSTR DriverName){
// 	if(!DriverName) return NULL;
// 	UINT16 len = wstrlen(DriverName);
// 	if(!len || len > MAX_DISPLAY_NAME_LENGTH) return NULL;
// 	DRIVER_LIST* List = &DriverList;
// 	for(;;){
// 		for(UINT i = 0;i<UNITS_PER_LIST;i++){
// 			if(List->Drivers[i].LenDriverName == len && List->Drivers[i].Present
// 			&& wstrcmp_nocs(List->Drivers[i].DriverName,DriverName, List->Drivers[i].LenDriverName)
// 			){
// 				PROCESS* Process = GetCurrentProcess();
// 				HANDLE hDriver = NULL;
// 				if(!(hDriver = OpenHandle(
// 					Process->Handles, NULL, 0, HANDLE_OPEN_DRIVER,
// 					&List->Drivers[i], NULL
// 				))) SET_SOD_PROCESS_MANAGEMENT;

// 				if(Process->SegmentBase == SEGBASE_USER){
// 					return (RFDRIVER)hDriver;
// 				}

// 				return &List->Drivers[i];
// 			}
// 		}
// 		if(!List->Next) break;
// 		List = List->Next;
// 	}
// 	return NULL;
// }
// BOOL CloseDriver(RFDRIVER Driver){
// 	return FALSE;
// }

// RFDCI_ACCESS_PORT CreateAccessPort(
// 	RFDRIVER Driver,
// 	UINT64 UniquePortId,
// 	LPWSTR PortName,
// 	UINT AccessPolicy,
// 	RFSERVER AlternativeServer
// ){
// 	// Check Access Policy Validation
// 	if(AccessPolicy == INVALID_ACCESS_POLICY || 
// 	(AccessPolicy & (INTERFACE_ACCESS_USER | INTERFACE_ACCESS_PRIVILEGED_USER))
// 	== (INTERFACE_ACCESS_USER | INTERFACE_ACCESS_PRIVILEGED_USER)
// 	) return NULL;
// 	if(AccessPolicy & INTERFACE_ACCESS_PRIVATE_ONLY &&
// 	AccessPolicy != INTERFACE_ACCESS_PRIVATE_ONLY
// 	) return NULL;


// 	if(!isValidDriver(Driver)) return NULL;
// 	__SpinLockSyncBitTestAndSet(&Driver->LockTable.LockChain0, DRV_LOCKCHAIN0_ACCESS_PORTS);
// 	RFDCI_ACCESS_PORT AccessPort = Driver->Ports;
// 	if(AlternativeServer && !isValidServer(AlternativeServer)) return NULL;
// 	for(;;){
// 		if(AccessPort->UniquePortId == UniquePortId){
// 			__BitRelease(&Driver->LockTable.LockChain0, DRV_LOCKCHAIN0_ACCESS_PORTS);
// 			return NULL;
// 		}
// 		if(!AccessPort->NextPort) break;
// 		AccessPort = AccessPort->NextPort;
// 	}
// 	AccessPort = Driver->Ports;
// 	for(UINT64 PortId = 0;;PortId++){
// 		if(!AccessPort->Present){
// 			AccessPort->Present = TRUE;
// 			AccessPort->Driver = Driver;
// 			AccessPort->PortId = PortId;
// 			AccessPort->UniquePortId = UniquePortId;
// 			if(PortName){
// 				UINT16 LenPortName = wstrlen(PortName);
// 				LPWSTR PortNameCopy = kmalloc((LenPortName + 1) << 1);
// 				memcpy(PortNameCopy, PortName, LenPortName);
// 				PortNameCopy[LenPortName] = 0;
// 				AccessPort->PortName = PortNameCopy;
// 				AccessPort->LenPortName = LenPortName;
// 			}
// 			AccessPort->AccessPolicy = AccessPolicy;
// 			AccessPort->AlternativeServer = AlternativeServer;
// 			__BitRelease(&Driver->LockTable.LockChain0, DRV_LOCKCHAIN0_ACCESS_PORTS);

// 			return AccessPort;
// 		}
// 		if(!AccessPort->NextPort){
// 			AccessPort->NextPort = kmalloc(sizeof(DCI_ACCESS_PORT));
// 			SZeroMemory(AccessPort->NextPort);
// 		}
// 		AccessPort = AccessPort->NextPort;
// 	}
	
// 	return NULL;
// }
// BOOL KERNELAPI isValidAccessPort(RFDCI_ACCESS_PORT AccessPort){
// 	if(!AccessPort || !AccessPort->Present || !isValidDriver(AccessPort->Driver)) return FALSE;
// 	DCI_ACCESS_PORT* DriverPort = AccessPort->Driver->Ports;
// 	for(UINT64 i = 0;i<AccessPort->PortId;i++) {
// 		if(!DriverPort->NextPort) return FALSE;
// 		DriverPort = DriverPort->NextPort;
// 	}
// 	if(DriverPort != AccessPort) return FALSE;
	
// 	return TRUE;
// }
// BOOL SetPortInterface(RFDCI_ACCESS_PORT AccessPort, UINT NumProcedures){
// 	if(!isValidAccessPort(AccessPort) || !NumProcedures || NumProcedures > MAX_INTERFACE_PROCS) return FALSE;
// 	if(AccessPort->Interface) kfree(AccessPort->Interface);
// 	UINT PortSize = sizeof(DCI_PROCEDURE_INTERFACE) + sizeof(DCI_PROCEDURE_OBJECT) * NumProcedures;
// 	AccessPort->Interface = kmalloc(PortSize);
// 	if(!AccessPort->Interface) SET_SOD_MEMORY_MANAGEMENT;
// 	SZeroMemory(AccessPort->Interface);
// 	AccessPort->Interface->NumProcedures = NumProcedures;
// 	return TRUE;
// }

// RFDCI_PROCEDURE_OBJECT RegisterInterfaceProcedure(
// 	RFDCI_ACCESS_PORT AccessPort,
// 	DCI_PROCEDURE Procedure,
// 	UINT64 ProcedureUniqueId,
// 	LPWSTR ProcedureName,
// 	UINT64 MinPacketLength,
// 	UINT64 MaxPacketLength
// ){
// 	if(!Procedure || !isValidAccessPort(AccessPort) || !AccessPort->Interface ||
// 	AccessPort->Interface->NumProcedures == AccessPort->Interface->NumRegisteredProcedures
// 	|| GetProcedureByUniqueId(AccessPort, ProcedureUniqueId) ||
// 	GetProcedureByName(AccessPort, ProcedureName)
// 	)
// 		return NULL;
	
// 	DCI_PROCEDURE_OBJECT* ProcedureObject = NULL;
// 	for(UINT i = 0;i<AccessPort->Interface->NumProcedures;i++){
// 		if(!AccessPort->Interface->Procs[i].Present){
// 			ProcedureObject = &AccessPort->Interface->Procs[i];
// 			ProcedureObject->Present = TRUE;
// 			ProcedureObject->AccessPort = AccessPort;
// 			ProcedureObject->ProcedureId = i;
// 			if(ProcedureName){
// 				UINT16 LenProcName = wstrlen(ProcedureName);
// 				LPWSTR ProcNameCopy = kmalloc((LenProcName + 1) << 1);
// 				memcpy(ProcNameCopy, ProcedureName, LenProcName << 1);
// 				ProcNameCopy[LenProcName] = 0;
// 				ProcedureObject->LenProcedureName = LenProcName;
// 				ProcedureObject->ProcedureName = ProcNameCopy;
// 			}
// 			ProcedureObject->UniqueProcedureId = ProcedureUniqueId;
// 			ProcedureObject->MinPacketLength = MinPacketLength;
// 			ProcedureObject->MaxPacketLength = MaxPacketLength;
// 			ProcedureObject->Procedure = Procedure;
// 			return ProcedureObject;
// 		}
// 	}
// 	return NULL;
// }


// RFDCI_PROCEDURE_OBJECT GetProcedureByUniqueId(RFDCI_ACCESS_PORT Port, UINT64 ProcedureUniqueId){
// 	return NULL;
// }
// RFDCI_PROCEDURE_OBJECT GetProcedureByName(RFDCI_ACCESS_PORT Port, LPWSTR ProcedureName){
// 	return NULL;
// }