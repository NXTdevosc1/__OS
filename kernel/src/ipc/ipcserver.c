#include <ipc/ipcserver.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <stdlib.h>
#include <preos_renderer.h>

__declspec(align(0x1000)) SERVER_LIST ServerList = { 0 };

KERNELSTATUS KERNELAPI		 IpcServerCreate(RFTHREAD Thread, UINT64 ServerIpAddress, LPWSTR ServerPassword, QWORD AccessPolicy, RFSERVER* PRFServer){
	if (!Thread || !PRFServer) {
		*PRFServer = NULL;
		return KERNEL_SERR_INVALID_PARAMETER;
	}
	if (ServerIpAddress && IpcGetServer(ServerIpAddress)) {
		*PRFServer = NULL;
		return KERNEL_SERR_ADDRESS_ALREADY_TAKEN;
	}
	UINT PasswordLength = 0;
	if (ServerPassword) {
		PasswordLength = wstrlen(ServerPassword);
		if (!PasswordLength || PasswordLength > MAX_SERVER_PASSWORD_LENGTH) return KERNEL_SERR_INVALID_PARAMETER;
	}
	SERVER_LIST* List = &ServerList;
	for (UINT64 ListId = 0;; ListId++) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!List->Servers[i].Present) {
				RFSERVER Server = &List->Servers[i];
				Server->Present = TRUE;
				
				IpcClientCreate(Thread, 0, 0);
				GP_clear_screen(0xffff);
				while(1);
				Server->ServerId = ListId * UNITS_PER_LIST + i;
				Server->ServerAddress = ServerIpAddress;
				Server->ServerHandle = OpenHandle(Thread->Process->Handles, Thread, 0, HANDLE_SERVER, (void*)Server, NULL);
				if (!Server->ServerHandle) SET_SOD_PROCESS_MANAGEMENT;
				
				Server->__ParentList = List;
				Server->__InListIndex = i;
				
				if (ServerPassword) {
					
					LPWSTR PasswordCopy = AllocatePool((PasswordLength + 1) << 1);
					if (!PasswordCopy) SET_SOD_MEMORY_MANAGEMENT;
					memcpy(PasswordCopy, ServerPassword, PasswordLength << 1);
					PasswordCopy[PasswordLength] = 0;
					Server->ServerPassword = PasswordCopy;
					Server->LenServerPassword = PasswordLength;
				}

				Server->Host->ServerClient = TRUE;
				Server->Host->Server = Server;
				Server->AccessPolicy = AccessPolicy;
				*PRFServer = Server;
				return KERNEL_SOK;
			}
		}
		if (!List->Next) {
			List->Next = AllocatePool(sizeof(SERVER_LIST));
			SZeroMemory(List->Next);
		}
		List = List->Next;
	}
	return KERNEL_SERR;
}

BOOL KERNELAPI isValidServer(RFSERVER Server) {
	if (!Server || Server->Present != 1 || !Server->__ParentList ||
		&Server->__ParentList->Servers[Server->__InListIndex] != Server
		) return FALSE;

	return TRUE;
}

BOOL KERNELAPI		 IpcServerDestroy(RFSERVER Server) {
	if (!isValidServer(Server)) return FALSE;
	Server->Present = 2; // ready for destroy
	IpcClientDestroy(Server->Host);
	CloseHandle(Server->ServerHandle);
	RFSERVER_CLIENT_LIST ClientList = &Server->ConnectedClients;

	for (;;) {

	}
}

BOOL KERNELAPI IpcServerSetStatus(RFSERVER Server, KERNELSTATUS Status) {
	if (!isValidServer(Server)) return FALSE;
	return IpcSetStatus(Server->Host, Status);
}



BOOL KERNELAPI isConnectedClient(RFSERVER Server, PCLIENT Client) {
	if (!Client || !isValidServer(Server)) return FALSE;
	RFSERVER_CLIENT_LIST ClientList = &Server->ConnectedClients;
	for (;;) {
		for (UINT i = 0; i < UNITS_PER_LIST; i++) {
			if (ClientList->Clients[i] == Client) return TRUE;
		}
		if (!ClientList->Next) break;
		ClientList = ClientList->Next;
	}
	return FALSE;
}

KERNELSTATUS KERNELAPI		 IpcSendToServer(PCLIENT Client, RFSERVER Server, BOOL Async, MSG* Message) {
	if (!isConnectedClient(Server, Client) || !Message) return KERNEL_SERR_INVALID_PARAMETER;
	return IpcSendMessage(Client, Server->Host, Async, Message->Message, Message->Data, Message->NumBytes);
}

KERNELSTATUS KERNELAPI IpcServerSend(RFSERVER Server, PCLIENT Client, BOOL Async, MSG* Message) {
	if (!isConnectedClient(Server, Client) || !Message) return KERNEL_SERR_INVALID_PARAMETER;
	return IpcSendMessage(Server->Host, Client, Async, Message->Message, Message->Data, Message->NumBytes);
}

BOOL KERNELAPI		 IpcServerReceive(RFSERVER Server, MSG_OBJECT* Message) {
	if (!Server || !Message) return FALSE;
	MSG Dump = { 0 };
	BOOL Status = IpcGetMessage(Server->Host, &Dump);
	if (Status && Server->Host->CurrentMessage) {
		memcpy(Message, Server->Host->CurrentMessage, sizeof(MSG_OBJECT));
	}
	return Status;
}
BOOL KERNELAPI		 IpcBroadcast(RFSERVER Server, BOOL Async, MSG* Message) {
	if (!isValidServer(Server) || !Message) return FALSE;
	RFSERVER_CLIENT_LIST ClientList = &Server->ConnectedClients;
	UINT64 NumClientsSent = 0;
	for (;;) {
		if (NumClientsSent >= Server->NumClients) break;
		for (UINT i = 0; i < UNITS_PER_LIST; i++) {
			if (NumClientsSent >= Server->NumClients) break;
			if (ClientList->Clients[i]) {
				IpcSendMessage(Server->Host, ClientList->Clients[i], Async,
					Message->Message, Message->Data, Message->NumBytes
				);
				NumClientsSent++;
			}
		}
		if (!ClientList->Next) break;
		ClientList = ClientList->Next;
	}
	return TRUE;
}

RFSERVER KERNELAPI		 IpcGetServer(UINT64 ServerIpAddress) {
	if (!ServerIpAddress) return NULL;
	RFSERVER_LIST List = &ServerList;
	for (;;) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (List->Servers[i].ServerAddress == ServerIpAddress &&
				List->Servers[i].Present == 1) {
				return &List->Servers[i];
			}
		}
		if (!List->Next) break;
		List = List->Next;
	}
	return NULL;
}

RFSERVER KERNELAPI		 IpcServerConnect(UINT64 ServerIpAddress, PCLIENT Client, LPWSTR ServerPassword) {
	if (!IpcIsValidClient(NULL, Client)) return NULL;
	RFSERVER Server = IpcGetServer(ServerIpAddress);
	if (!Server) return NULL;
	if (isConnectedClient(Server, Client)) return Server; // Client is already connected

	if (Server->ServerPassword) {
		if (!ServerPassword || !memcmp(Server->ServerPassword, ServerPassword, Server->LenServerPassword))
			return NULL;
	} 

	// Allocate client for server
	RFSERVER_CLIENT_LIST List = &Server->ConnectedClients;
	for (;;) {
		for (UINT64 i = 0; i < UNITS_PER_LIST; i++) {
			if (!List->Clients[i]) {
				List->Clients[i] = Client;
				HANDLE ConnectionHandle = OpenHandle(Client->Process->Handles,
					Client->Thread, 0, HANDLE_SERVER_CONNECTION, Server, NULL
				);
				if (!ConnectionHandle) SET_SOD_PROCESS_MANAGEMENT;
				ConnectionHandle->Data1 = Client;
				Server->NumClients++;
				return Server; // Successfull connection
			}
		}
		if (!List->Next) {
			List->Next = AllocatePool(sizeof(SERVER_CLIENT_LIST));
			SZeroMemory(List->Next);
		}
		List = List->Next;
	}
	return NULL;
}
BOOL KERNELAPI		 IpcServerDisconnect(RFSERVER Server, PCLIENT Client){
	return FALSE;
}

BOOL KERNELAPI		 IpcServerMessageDispatch(RFSERVER Server) {
	if (!Server) return FALSE;
	return IpcMessageDispatch(Server->Host);
}