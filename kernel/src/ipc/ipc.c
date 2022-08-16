#include <ipc/ipc.h>
#include <CPU/process.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <CPU/cpu.h>
#include <kernel.h>
#include <Management/debug.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <ipc/ipcserver.h>

__declspec(align(0x1000)) CLIENT_LIST_TABLE ClientTable = { 0 };

void IpcInit(){
	UINT64 NumPages = ALIGN_VALUE(sizeof(CLIENT_LIST_TABLE), 0x1000) >> 12;
	MapPhysicalPages(kproc->PageMap, &ClientTable, &ClientTable, NumPages, PM_MAP | PM_CACHE_DISABLE);
}

PCLIENT AllocateClient() {
#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : AllocateClient");
#endif //  ___KERNEL_DEBUG___
	RFCLIENT_LIST List = &ClientTable.Clients;
	for (UINT64 ListId = 0;;ListId++) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!List->Clients[i].Present) {
				List->Clients[i].Present = 2;
				ClientTable.NumClients++;
				List->Clients[i].ClientId = ListId * UNITS_PER_LIST + i;
				List->Clients[i].__IndexInList = i;
				List->Clients[i].__ParentList = List;

				return &List->Clients[i];
			}
		}
		if (!List->Next) {
			UINT64 NumPages = ALIGN_VALUE(sizeof(CLIENT_LIST), 0x1000) >> 12;
			List->Next = AllocatePoolEx(NULL, NumPages, 0x1000, 0);
			if (!List->Next) SET_SOD_MEMORY_MANAGEMENT;
			SZeroMemory(List->Next);
			MapPhysicalPages(kproc->PageMap, List->Next, List->Next, NumPages, PM_MAP | PM_CACHE_DISABLE);
		}
		List = List->Next;
	}
	return NULL;
}

PCLIENT KERNELAPI IpcClientCreate(HTHREAD HostThread, UINT MessageQueueLength, UINT CreationFlags){
	if (MessageQueueLength < 50) MessageQueueLength = 0x2000;

#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : CreateClient");
#endif //  ___KERNEL_DEBUG___

	RFPROCESS Process = NULL;
	if (HostThread) {
		Process = HostThread->Process;
	}
	else {
		HostThread = KeGetCurrentThread();
		Process = HostThread->Process;
	}


	PCLIENT Client = AllocateClient();

	Client->Process = Process;
	Client->Thread = HostThread;

	UINT64 NumPages = ALIGN_VALUE(sizeof(struct _MSG_QUEUE) + sizeof(MSG_OBJECT) * (UINT64)MessageQueueLength, 0x1000) >> 12;

				
	Client->InMessageQueue = AllocatePoolEx(NULL, NumPages << 12, 0x1000, 0);
	if(!Client->InMessageQueue) SET_SOD_MEMORY_MANAGEMENT;

	ZeroMemory(Client->InMessageQueue, NumPages << 12);


	Client->InMessageQueue->MaxMessages = MessageQueueLength;
	Client->InMessageQueue->PossibleNextMessage = (UINT32)-1;
	MapPhysicalPages(kproc->PageMap, Client->InMessageQueue, Client->InMessageQueue, NumPages, PM_MAP | PM_CACHE_DISABLE);
	HANDLE Handle = OpenHandle(Process->Handles, HostThread, 0,
		HANDLE_CLIENT, Client, NULL
	);
	if (!Handle) SET_SOD_PROCESS_MANAGEMENT;
	Client->ClientHandle = Handle;
	Client->Present = TRUE; // declare its full presence
	#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : Client Successfully Created");
#endif //  ___KERNEL_DEBUG___
	
	return Client;
}
BOOL KERNELAPI IpcIsValidClient(RFPROCESS Process, PCLIENT Client) {
	if (!Client || !Client->Present || !Client->InMessageQueue || &Client->__ParentList->Clients[Client->__IndexInList] != Client) return FALSE;

	if (Process && Client->Process != Process) return FALSE;

	return TRUE;
}

BOOL KERNELAPI	IpcClientDestroy(PCLIENT Client) {
	if (!IpcIsValidClient(NULL, Client)) return FALSE;
#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : Destroy Client");
#endif //  ___KERNEL_DEBUG___
	CloseHandle(Client->ClientHandle);
	kfree(Client->InMessageQueue);

	HANDLE_ITERATION_STRUCTURE Iterator = { 0 };
	StartHandleIteration(Client->Process->Handles, &Iterator);
	HANDLE Handle = NULL;
	// Data = Server, Data1 = Client
	while ((Handle = GetNextHandle(&Iterator))) {
		if (Handle->DataType == HANDLE_SERVER_CONNECTION && Handle->Data1 == Client) {
			IpcServerDisconnect((RFSERVER)Handle->Data, Client);
		}
	}
	EndHandleIteration(&Iterator);

	SZeroMemory(Client);
	return TRUE;
}

KERNELSTATUS KERNELAPI IpcPostThreadMessage(HTHREAD Thread, UINT64 Message, void* Data, UINT64 NumBytes){
	if (!Thread) return KERNEL_SERR_INVALID_PARAMETER;
	return IpcSendMessage(KeGetCurrentThread()->Client, Thread->Client, TRUE, Message, Data, NumBytes);
}

#define MAX_ALLOCATE_MESSAGE_ATTEMPTS 20

// Post Message in the client OutQueue
KERNELSTATUS KERNELAPI IpcSendMessage(PCLIENT Source, PCLIENT Destination, BOOL Async, UINT64 Message, void* Data, UINT64 NumBytes)
{
#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : SendMessage");
#endif //  ___KERNEL_DEBUG___

	if (Source == Destination || !IpcIsValidClient(KeGetCurrentProcess(), Source)
		|| !IpcIsValidClient(NULL, Destination)) return KERNEL_SERR_INVALID_PARAMETER;

	MSG_OBJECT* Msg = NULL;
	MSG_QUEUE Queue = Destination->InMessageQueue;
	UINT i = 0;
	for (;;) {
		while(Queue->PendingMessages == Queue->MaxMessages) __Schedule();

		for (i = 0; i < Queue->MaxMessages; i++) {
			if (!Queue->Messages[i].Header.Set) {
				if(!__SyncBitTestAndSet(&Queue->Messages[i].Header.Set, 0)) continue;
				Msg = &Queue->Messages[i];
				__SpinLockSyncBitTestAndSet(&Destination->AccessLocked, 0);
				__cli(); // to increase throuput performance (no task scheduler lock required)
				// since it is some very basic instructions
				Msg->Header.MessageId = Queue->LastId;
				Queue->LastId++;
				__BitRelease(&Destination->AccessLocked, 0);
				__sti();
				__SyncIncrement32(&Queue->PendingMessages);
				break;
			}
		}
		if (Msg) {
			
			// Setting Up The Body
			Msg->Body.Message = Message;
			Msg->Body.Data = Data;
			Msg->Body.NumBytes = NumBytes;
			// Setting Up The Header
			KERNELSTATUS SyncStatus = KERNEL_SOK_NOSTATUS;
			if (!Async) {
				Msg->Header.Status = &SyncStatus;
			}
			if (Source->ServerClient) {
				Msg->Header.Server = Source->Server;
			}else Msg->Header.Source = Source;
			
			Msg->Header.Destination = Destination;
			Msg->Header.Pending = TRUE;
			if(Msg->Header.MessageId == Queue->LeastId){
				Queue->PossibleNextMessage = i;
			}
			IoFinish(Destination->Thread);
			// CpuManagementTable[Destination->Thread->UniqueCpu]->NextThread = Destination->Thread;

			if (!Async) {
				while (Msg->Header.Pending) {
					IoWait();
				}
				IoFinish(Source->Thread);
				return SyncStatus;
			}
			return KERNEL_SOK;
		}


	}

	return KERNEL_SERR_NOT_FOUND;

	
}

// Get Server/Process Message
BOOL KERNELAPI		 IpcGetMessage(PCLIENT Client, MSG* Message) {
	if (!Message) return FALSE;
	if (!Client) Client = KeGetCurrentThread()->Client;
	if (!IpcIsValidClient(NULL, Client)) return FALSE;
	RFPROCESS Process = Client->Process;
	if (Client->CurrentMessage) return FALSE;
	if (Process->OperatingMode != KERNELMODE_PROCESS) {
		Message = KeResolvePhysicalAddress(Process, Message);
		if (!Message) return FALSE;
	}

	MSG_QUEUE InQueue = Client->InMessageQueue;

	// Spinlock asynchronously until bit is free and sets it atomically
#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : Get Message.");
#endif //  ___KERNEL_DEBUG___

	
	for (;;) {
		if(!InQueue->PendingMessages && !Client->AccessLocked &&
		__SyncBitTestAndSet(&Client->AccessLocked, 0)){
			__cli();
			InQueue->LastId = 0;
			InQueue->LeastId = 0;
			__BitRelease(&Client->AccessLocked, 0);
			__sti();
		}
		while (!InQueue->PendingMessages) IoWait();
		// if finished before task switch
		MSG_OBJECT* SelectedMessage = NULL;
		register UINT LeastId = InQueue->LeastId;
		register UINT PossibleNextMessage = InQueue->PossibleNextMessage;

		if(PossibleNextMessage != (UINT32)-1){
			register MSG_OBJECT* MessageObject = &InQueue->Messages[PossibleNextMessage];
			if(MessageObject->Header.Pending &&
			MessageObject->Header.MessageId == LeastId
			){
				SelectedMessage = MessageObject;
			}
			InQueue->PossibleNextMessage = (UINT32)-1;
		}
		if(!SelectedMessage){
			register UINT NextPossibleMessageId = LeastId + 1;
			for (UINT i = 0; i < InQueue->MaxMessages; i++) {
				if (InQueue->Messages[i].Header.Pending
				) {
						SelectedMessage = &InQueue->Messages[i];
						break;


					if(InQueue->Messages[i].Header.MessageId == LeastId){
						SelectedMessage = &InQueue->Messages[i];
						break;
					}else if(InQueue->Messages[i].Header.MessageId == NextPossibleMessageId){
						InQueue->PossibleNextMessage = i;
					}
				}
			}
		}

		if (SelectedMessage) {
#ifdef  ___KERNEL_DEBUG___
			DebugWrite("IPC : Acquiring Message to callee.");
#endif //  ___KERNEL_DEBUG___
			MSG* QueueMessage = &SelectedMessage->Body;
			memcpy(Message, QueueMessage, sizeof(MSG));
			Client->CurrentMessage = SelectedMessage;
#ifdef ___KERNEL_DEBUG___
			DebugWrite("IPC : Message successfully acquired (MSG, Acquired Message, Acquired Message Object)");
			DebugWrite(to_hstring64((UINT64)Message));
			DebugWrite(to_hstring64((UINT64)QueueMessage));
			DebugWrite(to_hstring64((UINT64)SelectedMessage));
#endif // ___KERNEL_DEBUG___
			return TRUE;
		}
	}
	return FALSE;
}


BOOL KERNELAPI IpcSetStatus(PCLIENT Client, KERNELSTATUS Status) {

	if (!IpcIsValidClient(KeGetCurrentProcess(), Client)) return FALSE;

	while (!Client->CurrentMessage || !Client->InMessageQueue->PendingMessages);

	if(Client->CurrentMessage->Header.Status)
		*Client->CurrentMessage->Header.Status = Status;

	return TRUE;
}


BOOL KERNELAPI IpcMessageDispatch(PCLIENT Client) {
	// if (!IpcIsValidClient(KeGetCurrentProcess(), Client)) return FALSE;

	if (!Client->CurrentMessage || !Client->InMessageQueue->PendingMessages) return FALSE;

	HTHREAD SenderThread = Client->CurrentMessage->Header.Source->Thread;

	BOOL SyncMessage = FALSE;
	if (Client->CurrentMessage->Header.Status) SyncMessage = TRUE;


	Client->CurrentMessage->Header.Pending = 0;
	Client->CurrentMessage->Header.Set = 0;
	Client->CurrentMessage = NULL;
	Client->InMessageQueue->LeastId++;
	__SyncDecrement32(&Client->InMessageQueue->PendingMessages);
	// if thread is suspended (Synchronous Message)
	if(SyncMessage)
		IoFinish(SenderThread);


#ifdef  ___KERNEL_DEBUG___
	DebugWrite("IPC : Message Successfully Dispatched.");
#endif //  ___KERNEL_DEBUG___
	return TRUE;
}

BOOL KERNELAPI IpcQueryMessageHeader(PCLIENT Client, MSG_HEADER* Header){
	if(!IpcIsValidClient(NULL, Client) || !Header || !Client->CurrentMessage) return FALSE;
	memcpy(Header, &Client->CurrentMessage->Header, sizeof(MSG_HEADER));
	return TRUE;
}

void* KERNELAPI IpcGetMessagePacket(PCLIENT Client){
	if(!IpcIsValidClient(NULL, Client) || !Client->CurrentMessage) return NULL;
	void* Packet = Client->CurrentMessage->Body.Data;
	if(Client->CurrentMessage->Header.Source->Process->OperatingMode == USERMODE_PROCESS){
		Packet = KeResolvePhysicalAddress(Client->CurrentMessage->Header.Source->Process, Packet);
	}
	return Packet;
}

PCLIENT KERNELAPI IpcGetThreadClient(HTHREAD Thread) {
	if (!Thread) return NULL;
	return Thread->Client;
}