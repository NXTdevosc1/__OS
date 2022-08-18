#pragma once
#include <krnltypes.h>
#include <CPU/process_defs.h>
#include <Management/lock.h>
#define IPC_RESEND(x) ((KERNELSTATUS)(x) == KERNEL_SWR_RESEND)

#define IPC_MAKEADDRESS(Data0, Data1, Data2, Data3) (((UINT64)(Data0 & 0xFFFF) << 48) | ((UINT64)(Data1 & 0xFFFF) << 32) | (UINT64)((Data2 & 0xFFFF) << 16) | (UINT16)(Data3))


typedef struct _MSG MSG;
typedef struct _CLIENT CLIENT, *PCLIENT;
typedef struct _MSG_OBJECT MSG_OBJECT;

typedef struct _MSG_HEADER MSG_HEADER;



typedef struct _MSG_QUEUE *MSG_QUEUE;

typedef struct _CLIENT_LIST CLIENT_LIST, * RFCLIENT_LIST;

typedef struct _SERVER SERVER, * RFSERVER;

typedef struct _MSG_HEADER {
	BOOL	Set;
	BOOL	Pending;
	UINT64  MessageId;
	PCLIENT Source;
	RFSERVER Server; // if set then ignore source
	PCLIENT Destination;
	KERNELSTATUS* Status;
} MSG_HEADER;

typedef struct _MSG {
	UINT64	Message; // a simple (optionnal) descriptor or id for the message
	void* Data;
	UINT64 NumBytes;
} MSG;

typedef struct _MSG_OBJECT {
	MSG_HEADER	Header;
	MSG			Body;
} MSG_OBJECT;

typedef struct _MSG_QUEUE {
	UINT	   MaxMessages;
	UINT	   PendingMessages;
	UINT64     LastId; // Next message id, set to 0 when no pending messages
	UINT64	   LeastId;
	UINT	   PossibleNextMessage; // Index of message if id == list (maybe incorrect)
	MSG_OBJECT Messages[];
} *MSG_QUEUE;



typedef struct _CLIENT {
	BOOL		Present; // set to 2 when creating the client
	UINT64		AccessLocked; // Set by IPC Thread to modify information
	UINT64		ClientId;
	UINT64			__IndexInList;
	RFCLIENT_LIST	__ParentList;
	RFPROCESS	Process;
	RFTHREAD		Thread;
	HANDLE      ClientHandle;
	MSG_QUEUE	InMessageQueue;
	MSG_OBJECT* CurrentMessage;
	BOOL		ServerClient; // Is it a client server
	RFSERVER	Server; // Set as destination to keep client private
} CLIENT, *PCLIENT;




typedef struct _CLIENT_LIST {
	CLIENT Clients[UNITS_PER_LIST];
	RFCLIENT_LIST Next;
} CLIENT_LIST, *RFCLIENT_LIST;

typedef struct _CLIENT_LIST_TABLE {
	UINT64 NumClients;
	CLIENT_LIST Clients;
} CLIENT_LIST_TABLE, *RFCLIENT_LIST_TABLE;

void IpcInit();

BOOL KERNELAPI IpcIsValidClient(RFPROCESS Process, PCLIENT Client);

PCLIENT KERNELAPI IpcClientCreate(RFTHREAD HostThread, UINT MessageQueueLength, UINT CreationFlags);
BOOL KERNELAPI	IpcClientDestroy(PCLIENT Client);

// PCLIENT Destination. Will be cleared after the message was processed

KERNELSTATUS KERNELAPI IpcSendMessage(PCLIENT Source, PCLIENT Destination, BOOL Async, UINT64 Message, void* Data, UINT64 NumBytes);

BOOL KERNELAPI IpcSetStatus(PCLIENT Client, KERNELSTATUS Status);
BOOL KERNELAPI IpcMessageDispatch(PCLIENT Client);

//KERNELSTATUS KERNELAPI AsyncSendMessage(PCLIENT Client, PCLIENT Destination, MSG* Message, KERNELSTATUS* ReturnValue, BOOL* Pending);


KERNELSTATUS KERNELAPI IpcPostThreadMessage(RFTHREAD Thread, UINT64 Message, void* Data, UINT64 NumBytes);



// Server Perspective
// User Perspective




// Get Server/Process Message
BOOL KERNELAPI		 IpcGetMessage(PCLIENT Client, MSG* Message);

//void KERNELAPI KTIpcClientThread(); // Kernel Thread (KT)
PCLIENT KERNELAPI IpcGetThreadClient(RFTHREAD Thread);

BOOL KERNELAPI IpcQueryMessageHeader(PCLIENT Client, MSG_HEADER* Header);
void* KERNELAPI IpcGetMessagePacket(PCLIENT Client);