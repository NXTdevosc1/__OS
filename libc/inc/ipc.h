#pragma once
#include <usertypes.h>
#include <proghdr.h>

typedef struct _MSG {
	HANDLE      Client;
	HPROCESS	Process;
	HTHREAD		Thread;
	BOOL		Broadcast;
	DWORD		Time;
	UINT64		Message; //
	UINT64		LParam; // Low Param
	UINT64		HParam; // High Param & Return Value
	void*		DParam; // Data Param
} MSG;

HCLIENT eXTAPI CreateClient(HTHREAD HostThread, UINT ClientQueueLength);