#pragma once
#define __DLL_EXPORTS
#include <kernelruntime.h>
#include <sysipc.h>
#include <ipcserver.h>
#include <pciexpressapi.h>
#include <ddk.h>

RFPROCESS KERNELAPI KeCreateProcess(RFPROCESS ParentProcess, LPWSTR ProcessName, UINT64 Subsystem, UINT16 SegmentBase){
    return 0;
}
RFPROCESS KERNELAPI KeGetProcessById(UINT64 ProcessId){
    return 0;
}

BOOL KERNELAPI KeSetProcessName(RFPROCESS Process, LPWSTR ProcessName){ return 0; }



RFTHREAD KERNELAPI KeCreateThread(RFPROCESS Process, UINT64 StackSize, THREAD_START_ROUTINE StartAddress, UINT64 Flags, UINT64* LpThreadId, UINT8 NumParameters, ...){ return 0; } // parameters
HRESULT KERNELAPI KeSetThreadPriority(RFTHREAD Thread, int Priority){ return 0; }
HRESULT KERNELAPI KeSetPriorityClass(RFPROCESS Process, int PriorityClass){ return 0; }


void KERNELAPI KeTerminateCurrentProcess(int ExitCode){}
void KERNELAPI KeTerminateCurrentThread(int ExitCode){}
int KERNELAPI KeTerminateProcess(RFPROCESS process, int ExitCode){ return 0; }
int KERNELAPI KeTerminateThread(RFTHREAD thread, int ExitCode){ return 0; }

RFPROCESS KERNELAPI KeGetCurrentProcess(){ return 0; }
RFTHREAD KERNELAPI KeGetCurrentThread(){ return 0; }

UINT64 KERNELAPI KeGetCurrentProcessId(){ return 0; }
UINT64 KERNELAPI KeGetCurrentThreadId(){ return 0; }

BOOL KERNELAPI KeSuspendThread(RFTHREAD Thread){ return 0; }
BOOL KERNELAPI KeResumeThread(RFTHREAD Thread){ return 0; }

void KERNELAPI KeSetDeathScreen(UINT64 DeathCode, UINT16* Msg, UINT16* Description, UINT16* SupportLink){while(1);}
PVOID KERNELAPI KeGetRuntimeSymbol(char* SymbolName){return NULL;}
void KERNELAPI KeTaskSchedulerEnable(){while(1);}
void KERNELAPI KeTaskSchedulerDisable(){while(1);}

PVOID KERNELAPI malloc(unsigned long long Size){return NULL;}
PVOID KERNELAPI KeExtendedAlloc(RFTHREAD Thread, UINT64 NumBytes, UINT32 Align, PVOID* AllocationSegment, UINT64 MaxAddress) {return NULL;}
PVOID KERNELAPI free(const void* Heap){return NULL;}


BOOL KERNELAPI IpcIsValidClient(RFPROCESS Process, RFCLIENT Client){return FALSE;}
RFCLIENT KERNELAPI IpcClientCreate(RFTHREAD Host, UINT MessageQueueLength, DWORD CreationFlags)
{return NULL;}
BOOL KERNELAPI IpcClientDestroy(RFCLIENT Client){return 0;}
KERNELSTATUS KERNELAPI IpcSendMessage(
    RFCLIENT    Source,
    RFCLIENT    Destination,
    BOOL        Async,
    UINT64      Message,
    void*       Data,
    UINT64      NumBytes
){return 0;}

// Set status for the current Message
BOOL KERNELAPI IpcSetCallStatus(RFCLIENT Client, KERNELSTATUS Status){return 0;}
BOOL KERNELAPI IpcMessageDispatch(RFCLIENT Client){return 0;}

KERNELSTATUS KERNELAPI IpcPostThreadMessage(RFTHREAD Thread, UINT64 Message, void* Data, UINT64 NumBytes){return 0;}
BOOL KERNELAPI IpcGetMessage(RFCLIENT Client, MSG* Message){return 0;}



KERNELSTATUS KERNELAPI IpcServerCreate(
    RFTHREAD Thread,
    IPC_ADDRESS LocalServerAddress,
    LPWSTR ServerPassword,
    QWORD AccessPolicy,
    RFSERVER* PRFServer
){return 0;}

BOOL KERNELAPI IpcServerDestroy(RFSERVER Server){return 0;}
RFSERVER KERNELAPI IpcServerConnect(
    IPC_ADDRESS LocalServerAddress,
    RFCLIENT Client,
    LPWSTR   ServerPassword
){return 0;}

BOOL KERNELAPI IpcServerDisconnect(RFSERVER Server, RFCLIENT Client){return 0;}

KERNELSTATUS KERNELAPI IpcSendToServer(
    RFCLIENT    Client,
    RFSERVER    Server,
    BOOL        Async,
    MSG*        Message
){return 0;}

KERNELSTATUS KERNELAPI IpcServerSend(
    RFSERVER    Server,
    RFCLIENT    Client,
    BOOL        Async,
    MSG*        Message
){return 0;}

BOOL KERNELAPI  IpcServerReceive(
    RFSERVER Server,
    MSG_OBJECT* MessageObject
){return 0;}

BOOL KERNELAPI IpcBroadCast(
    RFSERVER Server,
    BOOL     Async,
    MSG*     Message
){return 0;}

BOOL KERNELAPI IpcServerMessageDispatch(RFSERVER Server){return 0;}

BOOL KERNELAPI IpcServerSetCallStatus(RFSERVER Server, KERNELSTATUS Status){return 0;}

RFSERVER KERNELAPI IpcGetServer(IPC_ADDRESS LocalServerAddress){return 0;}
BOOL    KERNELAPI  IpcIsValidServer(RFSERVER Server){return 0;}
BOOL    KERNELAPI  IpcIsConnectedClient(RFSERVER Server, RFCLIENT Client){return 0;}
RFCLIENT KERNELAPI IpcGetThreadClient(RFTHREAD Thread){return 0;}

// Optionnal Parameter Set to -1 if not set
RFPCI_CONFIGURATION_HEADER KERNELAPI PciExpressFindDevice(
    _IN UINT16 VendorId,
    _IN _OPT UINT16 DeviceId
){return NULL;}

// Optionnal Parameter Set to -1 if not set
BOOL KERNELAPI PciExpressEnumerateDevices(
    _IN unsigned char PciClass,
    _IN _OPT unsigned char PciSubClass,
    _IN _OPT unsigned char ProgramInterface,
    _IN RFPCI_CONFIGURATION_HEADER* PciConfigurationPtrList,
    _IN DWORD Max
){return FALSE;}

RFPCI_CONFIGURATION_HEADER KERNELAPI PciExpressConfigurationRead(
    _IN UINT16 PciExpressDevice,
    _IN unsigned char Bus,
    _IN unsigned char Device,
    _IN unsigned char Function
){return NULL;}

BOOL KERNELAPI CheckPciExpress(){return FALSE;} // check if system is PCIE Compatible

BOOL KERNELAPI MapPhysicalPages(RFPROCESS Process, void* VirtualAddress, void* PhysicalAddress, UINT64 NumPages, UINT64 Flags){
    return FALSE;
}

BOOL KERNELAPI KeMapMemory(void* PhysicalAddress, UINT64 NumPages, UINT64 Flags){
    return FALSE;
}

KERNELSTATUS KERNELAPI SystemDebugPrint(LPCWSTR Format, ...){
    return KERNEL_SERR;
}

KERNELSTATUS KERNELAPI KeControlIrq(INTERRUPT_SERVICE_ROUTINE InterruptHandler, UINT IrqNumber, UINT DeliveryMode, UINT Flags){
    return KERNEL_SERR;
}
KERNELSTATUS KERNELAPI KeReleaseIrq(UINT IrqNumber){
    return KERNEL_SERR;
}

void KERNELAPI Sleep(UINT64 Milliseconds){}
void KERNELAPI MicroSleep(UINT64 MicroSeconds){}

KERNELSTATUS KERNELAPI SetInterruptService(RFDEVICE_OBJECT Device, INTERRUPT_SERVICE_ROUTINE InterruptHandler) {
    return KERNEL_SERR;
}

BOOL KERNELAPI IoWait() {
    return FALSE;
}
BOOL KERNELAPI IoFinish(HTHREAD Thread) {
    return FALSE;
}