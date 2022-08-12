#include <sys/KeImplementation.h>
#include <ipc/ipcserver.h>
// RFPROCESS KEXAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR DisplayName, LPWSTR DisplayDescription, UINT SubSystem, UINT64 Privileges, UINT64* ProcessId){
// 	RFPROCESS Process = GetCurrentProcess();
// 	RFPROCESS ReturnedProcess = NULL;
// 	CREATE_PROCESS_STRUCTURE CreateProcessStructure = {
// 	Process, ParentProcess, DisplayName, DisplayDescription,
// 	SubSystem, Privileges, &ReturnedProcess, ProcessId
// 	};
// 	MSG MessageBody = { KEX_CREATE_PROCESS, (void*)&CreateProcessStructure, sizeof(CREATE_PROCESS_STRUCTURE)};
// 	IpcSendToServer(GetCurrentThread()->Client, KernelServer, FALSE, &MessageBody);
// 	return ReturnedProcess;
// }