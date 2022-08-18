#pragma once
#include <ipc/ipc.h>

typedef struct _SERVER SERVER, * RFSERVER;

typedef struct _SERVER_CLIENT_LIST SERVER_CLIENT_LIST, * RFSERVER_CLIENT_LIST;

typedef struct _SERVER_LIST SERVER_LIST, * RFSERVER_LIST;

typedef struct _SERVER_CLIENT_LIST {
	PCLIENT Clients[UNITS_PER_LIST];
	RFSERVER_CLIENT_LIST Next;
} SERVER_CLIENT_LIST, * RFSERVER_CLIENT_LIST;

#define MAX_SERVER_PASSWORD_LENGTH 0x1000

typedef struct _SERVER {
	BOOL	Present;
	UINT64	ServerId;
	HANDLE   ServerHandle;
	RFSERVER_LIST __ParentList;
	UINT		  __InListIndex;
	LPWSTR	ServerPassword;
	UINT16  LenServerPassword;
	PCLIENT Host;
	UINT64  ServerAddress;
	QWORD   AccessPolicy;
	UINT64   NumClients;
	SERVER_CLIENT_LIST ConnectedClients;
} SERVER, * RFSERVER;



typedef struct _SERVER_LIST {
	SERVER Servers[UNITS_PER_LIST];
	RFSERVER_LIST Next;
}SERVER_LIST, * RFSERVER_LIST;

enum _IPC_SERVER_ACCESS_POLICY {
	IPC_SERVER_KERNELMODE_ONLY = 1,
	IPC_SERVER_USERMODE_ONLY = 2,
	IPC_SERVER_USERMODE_SYSCALL_ACCESS = 4, // set if it is user mode only and access through syscalls is accepted
	IPC_SERVER_PRIVATE = 8, // Only accessible by the process itself or the processes that gets assigned by the host
};

KERNELSTATUS KERNELAPI		 IpcServerCreate(RFTHREAD Thread, UINT64 ServerIpAddress, LPWSTR ServerPassword, QWORD AccessPolicy, RFSERVER* PRFServer);
BOOL KERNELAPI		 IpcServerDestroy(RFSERVER Server);

RFSERVER KERNELAPI		 IpcServerConnect(UINT64 ServerIpAddress, PCLIENT Client, LPWSTR ServerPassword);
BOOL KERNELAPI		 IpcServerDisconnect(RFSERVER Server, PCLIENT Client);

KERNELSTATUS KERNELAPI		 IpcSendToServer(PCLIENT Client, RFSERVER Server, BOOL Async, MSG* Message);

KERNELSTATUS KERNELAPI IpcServerSend(RFSERVER Server, PCLIENT Client, BOOL Async, MSG* Message);

BOOL KERNELAPI		 IpcServerReceive(RFSERVER Server, MSG_OBJECT* Message);
BOOL KERNELAPI		 IpcBroadcast(RFSERVER Server, BOOL Async, MSG* Message);
BOOL KERNELAPI		 IpcServerMessageDispatch(RFSERVER Server);

BOOL KERNELAPI IpcServerSetStatus(RFSERVER Server, KERNELSTATUS Status);

RFSERVER KERNELAPI		 IpcGetServer(UINT64 ServerIpAddress);
BOOL KERNELAPI isValidServer(RFSERVER Server);
BOOL KERNELAPI isConnectedClient(RFSERVER Server, PCLIENT Client);