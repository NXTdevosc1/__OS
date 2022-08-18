// DYNAMIC COMMUNICATION INTERFACE (DCI)

#pragma once
#include <krnltypes.h>
#include <CPU/process_defs.h>
#include <ipc/ipc.h>
// #include <ipc/ipcserver.h>
// typedef struct _DCI_PROCEDURE_OBJECT DCI_PROCEDURE_OBJECT;
// typedef struct _DCI_ACCESS_PORT DCI_ACCESS_PORT, * RFDCI_ACCESS_PORT;
// typedef struct _DCI_PROCEDURE_INTERFACE DCI_PROCEDURE_INTERFACE, * PDCI_PROCEDURE_INTERFACE;
// typedef struct _SERVICE SERVICE, * RFSERVICE;
// typedef struct _DRIVER_LIST DRIVER_LIST;
// typedef struct _SERVICE_LIST SERVICE_LIST;
// typedef struct _DRIVER DRIVER, * RFDRIVER;

// #define DCIPROC __stdcall

// #define MAX_INTERFACE_PROCS 250
// #define MAX_DISPLAY_NAME_LENGTH 85
// typedef KERNELSTATUS(DCIPROC* DCI_PROCEDURE)(void* Packet, RFDCI_ACCESS_PORT Port);
// typedef KERNELSTATUS(__stdcall *DRIVER_UNLOAD_PROCEDURE)(RFDRIVER Driver);
// typedef KERNELSTATUS(__stdcall* DRIVER_STARTUP_PROCEDURE)(RFDRIVER Driver);

// #define MAX_PACKET_LENGTH ((UINT64)-1)

// typedef struct _DCI_PROCEDURE_OBJECT{
// 	BOOL Present;
// 	RFDCI_ACCESS_PORT AccessPort;
// 	UINT64 ProcedureId;
// 	LPWSTR ProcedureName; // Used to search procedure by name [Optionnal]
// 	UINT16 LenProcedureName;
// 	UINT64 UniqueProcedureId;
// 	UINT64 MinPacketLength;
// 	UINT64 MaxPacketLength;
// 	DCI_PROCEDURE Procedure;
// } DCI_PROCEDURE_OBJECT, * RFDCI_PROCEDURE_OBJECT;



// typedef struct _DCI_PROCEDURE_INTERFACE {
// 	UINT NumProcedures;
// 	UINT NumRegisteredProcedures;
// 	RFPROCESS CallerProcess; // Caller Process For Current Procedure
// 	RFTHREAD CallerThread;
// 	RFDCI_PROCEDURE_OBJECT CallerProcedureObject;
// 	UINT64 CallerPacketLength;
// 	void* CallerPacket;
// 	DCI_PROCEDURE_OBJECT Procs[]; // [NumProcedures]
// } DCI_PROCEDURE_INTERFACE, *PDCI_PROCEDURE_INTERFACE;


// enum DRIVER_LOCKCHAIN0{
// 	DRV_LOCKCHAIN0_ACCESS_PORTS = 0
// };

// typedef struct _DRIVER_LOCK_TABLE{
// 	UINT64 LockChain0; 
// } DRIVER_LOCK_TABLE;

// typedef struct _DRIVER{
// 	BOOL	Present;
// 	BOOL	Loaded;
// 	UINT64 DriverId;
// 	UINT64 DriverType;
// 	UINT64 VendorId;
// 	RFPROCESS Process;
// 	LPWSTR SystemPath;
// 	UINT16 LenDriverName;
// 	LPWSTR DriverName; // Process Name is how driver's name appears externally, this is like a key name
// 	DRIVER_STARTUP_PROCEDURE DriverStartup;
// 	DRIVER_UNLOAD_PROCEDURE DriverUnload;
// 	UINT NumDciPorts;
// 	RFDCI_ACCESS_PORT Ports; // Linked element list of ports
// 	HANDLE_TABLE* DeviceObjects;
// 	// UINT NumServices;
// 	// HANDLE_TABLE* ServiceHandles;
// 	DRIVER_LIST* ParentList;
// 	UINT InParentListIndex;
// 	DRIVER_LOCK_TABLE LockTable;
// } DRIVER, *RFDRIVER;

// typedef struct _DRIVER_LIST {
// 	DRIVER Drivers[UNITS_PER_LIST];
// 	DRIVER_LIST* Next;
// } DRIVER_LIST;



// typedef struct _SERVICE {
// 	UINT64 ServiceId;

// 	LPWSTR ServiceName;
// 	UINT64 ServiceType;
// 	DRIVER_UNLOAD_PROCEDURE ServiceUnload;
// 	RFDRIVER Driver;
// 	HANDLE Handle;
// } SERVICE, *RFSERVICE;

// typedef struct _SERVICE_LIST {
// 	SERVICE Services[UNITS_PER_LIST];
// 	SERVICE_LIST* Next;
// } SERVICE_LIST;

// enum DRIVER_INTERFACE_POLICY {
// 	INVALID_ACCESS_POLICY = 0, // 0 is Invalid
// 	INTERFACE_ACCESS_KERNEL = 1,
// 	INTERFACE_ACCESS_USER = 2, // Interface is accessible by all users
// 	INTERFACE_ACCESS_PRIVILEGED_USER = 4,
// 	INTERFACE_ACCESS_PRIVATE_ONLY = 8
// };

// // Used to make driver access multithread compatible
// typedef struct _DCI_ACCESS_PORT {
// 	BOOL	Present;
// 	RFDRIVER Driver;
// 	UINT64	PortId; // Used to validate the port
// 	UINT64	UniquePortId; // Secret Identifier
// 	LPWSTR PortName;
// 	UINT16 LenPortName;
// 	RFPROCESS Process;
// 	UINT	AccessPolicy;
// 	RFSERVER AlternativeServer;
// 	DCI_PROCEDURE_INTERFACE* Interface;
// 	RFDCI_ACCESS_PORT NextPort;
// } DCI_ACCESS_PORT, *RFDCI_ACCESS_PORT;

// BOOL isValidDriver(RFDRIVER Driver);
// RFDRIVER CreateDriver(DRIVER_STARTUP_PROCEDURE EntryPoint, UINT64 DriverType, LPWSTR SystemPath, LPWSTR DriverName);
// BOOL LoadDriver(RFPROCESS Process, RFDRIVER Driver);

// KERNELSTATUS UnloadDriver(RFDRIVER Driver);

// RFDRIVER OpenDriver(LPWSTR DriverName);
// BOOL CloseDriver(RFDRIVER Driver);

// BOOL KERNELAPI isValidAccessPort(RFDCI_ACCESS_PORT AccessPort);

// RFDCI_ACCESS_PORT CreateAccessPort(RFDRIVER Driver, UINT64 UniquePortId, LPWSTR PortName, UINT AccessPolicy, RFSERVER AlternativeServer); // To Communicate With Driver
// BOOL SetPortInterface(RFDCI_ACCESS_PORT AccessPort, UINT NumProcedures);
// RFDCI_PROCEDURE_OBJECT RegisterInterfaceProcedure(RFDCI_ACCESS_PORT AccessPort, DCI_PROCEDURE Procedure, UINT64 ProcedureUniqueId, LPWSTR ProcedureName, UINT64 MinPacketLength, UINT64 MaxPacketLength);

// BOOL UnregisterInterfaceProcedure(RFDCI_PROCEDURE_OBJECT Procedure);

// RFDCI_ACCESS_PORT OpenAccessPortId(RFDRIVER Driver, UINT64 UniquePortId);
// RFDCI_ACCESS_PORT OpenAccessPort(RFDRIVER Driver, LPWSTR PortName);

// BOOL CloseAccessPort(RFDCI_ACCESS_PORT AccessPort);

// RFSERVER AccessPortServerConnect(RFDCI_ACCESS_PORT Port);

// RFDCI_PROCEDURE_OBJECT GetProcedureByUniqueId(RFDCI_ACCESS_PORT Port, UINT64 ProcedureUniqueId);
// RFDCI_PROCEDURE_OBJECT GetProcedureByName(RFDCI_ACCESS_PORT Port, LPWSTR ProcedureName);
// KERNELSTATUS CallProcedure(RFDCI_ACCESS_PORT Port, RFDCI_PROCEDURE_OBJECT Procedure, UINT64 PacketLength, void* Packet);
