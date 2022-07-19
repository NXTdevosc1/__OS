#pragma once

#include <kerneltypes.h>
#include <krnlapi.h>
#define IPC_MAKEADDRESS(Data0, Data1, Data2, Data3) (((UINT64)(Data0 & 0xFFFF) << 48) | ((UINT64)(Data1 & 0xFFFF) << 32) | (UINT64)((Data2 & 0xFFFF) << 16) | (UINT16)(Data3))

typedef void* RFSERVER;
typedef void* RFCLIENT;

typedef struct _MSG_HEADER {
	BOOL	Set;
	BOOL	Pending;
	UINT64  MessageId;
	RFCLIENT Source;
	RFSERVER Server; // if set then ignore source
	RFCLIENT Destination;
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

RFCLIENT KERNELAPI IpcClientCreate(RFTHREAD Host, UINT MessageQueueLength, DWORD CreationFlags);
BOOL KERNELAPI IpcIsValidClient(RFPROCESS Process, RFCLIENT Client);
BOOL KERNELAPI IpcClientDestroy(RFCLIENT Client);
KERNELSTATUS KERNELAPI IpcSendMessage(
    RFCLIENT    Source,
    RFCLIENT    Destination,
    BOOL        Async,
    UINT64      Message,
    void*       Data,
    UINT64      NumBytes
);

// Set status for the current Message
BOOL KERNELAPI IpcSetCallStatus(RFCLIENT Client, KERNELSTATUS Status);
BOOL KERNELAPI IpcMessageDispatch(RFCLIENT Client);

KERNELSTATUS KERNELAPI IpcPostThreadMessage(RFTHREAD Thread, UINT64 Message, void* Data, UINT64 NumBytes);
BOOL KERNELAPI IpcGetMessage(RFCLIENT Client, MSG* Message);
RFCLIENT KERNELAPI IpcGetThreadClient(RFTHREAD Thread);