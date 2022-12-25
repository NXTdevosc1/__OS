#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Hello, world!\n");
    
    EFI_STATUS Status;
    EFI_BLOCK_IO* BlockIo;
    EFI_HANDLE* Handles = NULL;
    UINTN HandleSize = 0;
    CHAR16 Bf[0x40] = {0};
    Status = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleSize, Handles);
    if(Status != EFI_BUFFER_TOO_SMALL || HandleSize == 0) {
        StatusToString(Bf, Status);
        Print(L"Invalid Status or no efi handles. %ls\n", Bf);
        while(1);
    }
    Status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, HandleSize, (void**)&Handles);
    if(EFI_ERROR(Status)) {
        StatusToString(Bf, Status);
        Print(L"Allocation failed. %ls\n", Bf);
        while(1);
    }

    Status = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleSize, Handles);
    StatusToString(Bf, Status);
    UINTN NumHandles = HandleSize / sizeof(EFI_HANDLE);
    Print(L"Get handles success. %ls - %d\n", Bf, NumHandles);

    for(UINTN i = 0;i<NumHandles;i++) {
        Status = uefi_call_wrapper(BS->HandleProtocol, 3, Handles[i], &gEfiBlockIoProtocolGuid, (void**)&BlockIo);
        if(EFI_ERROR(Status)) {
            Print(L"Failed to get handle %d", i);
        }
        Print(L"Media ID : %u , BlockSize : %u, Present : %d, Removable : %d , SZ : %llu\n", 
        BlockIo->Media->MediaId, BlockIo->Media->BlockSize,
        BlockIo->Media->MediaPresent, BlockIo->Media->RemovableMedia,
        BlockIo->Media->LastBlock
        );
    }
    while(1);
    return EFI_SUCCESS;
}