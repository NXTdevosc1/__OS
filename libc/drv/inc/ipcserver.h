#pragma once
#include <krnlapi.h>
#include <kerneltypes.h>
#include <sysipc.h>

typedef void* RFSERVER;
typedef QWORD IPC_ADDRESS;
enum _IPC_SERVER_ACCESS_POLICY {
	IPC_SERVER_KERNELMODE_ONLY = 1,
	IPC_SERVER_USERMODE_ONLY = 2,
	IPC_SERVER_USERMODE_SYSCALL_ACCESS = 4, // set if it is user mode only and access through syscalls is accepted
	IPC_SERVER_PRIVATE = 8, // Only accessible by the process itself or the processes that gets assigned by the host
};

KERNELSTATUS KERNELAPI IpcServerCreate(
    RFTHREAD Thread,
    IPC_ADDRESS LocalServerAddress,
    LPWSTR ServerPassword,
    QWORD AccessPolicy,
    RFSERVER* PRFServer
);

BOOL KERNELAPI IpcServerDestroy(RFSERVER Server);
RFSERVER KERNELAPI IpcServerConnect(
    IPC_ADDRESS LocalServerAddress,
    RFCLIENT Client,
    LPWSTR   ServerPassword
);

BOOL KERNELAPI IpcServerDisconnect(RFSERVER Server, RFCLIENT Client);

KERNELSTATUS KERNELAPI IpcSendToServer(
    RFCLIENT    Client,
    RFSERVER    Server,
    BOOL        Async,
    MSG*        Message
);

KERNELSTATUS KERNELAPI IpcServerSend(
    RFSERVER    Server,
    RFCLIENT    Client,
    BOOL        Async,
    MSG*        Message
);

BOOL KERNELAPI  IpcServerReceive(
    RFSERVER Server,
    MSG_OBJECT* MessageObject
);

BOOL KERNELAPI IpcBroadCast(
    RFSERVER Server,
    BOOL     Async,
    MSG*     Message
);

BOOL KERNELAPI IpcServerMessageDispatch(RFSERVER Server);

BOOL KERNELAPI IpcServerSetCallStatus(RFSERVER Server, KERNELSTATUS Status);

RFSERVER KERNELAPI IpcGetServer(IPC_ADDRESS LocalServerAddress);
BOOL    KERNELAPI  IpcIsValidServer(RFSERVER Server);
BOOL    KERNELAPI  IpcCheckClientConnection(RFSERVER Server, RFCLIENT Client);
