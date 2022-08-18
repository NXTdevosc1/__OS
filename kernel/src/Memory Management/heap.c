#include <MemoryManagement.h>
#include <CPU/process.h>

void* AllocatePoolEx(RFPROCESS Process, UINT64 NumBytes, UINT Align, UINT64 Flags) {
    return NULL;
}
void* AllocatePool(UINT64 NumBytes) {
    return AllocatePoolEx(KeGetCurrentProcess(), NumBytes, 0, 0);
}
void* FreePool(void* HeapAddress) {
    return RemoteFreePool(KeGetCurrentProcess(), HeapAddress);
}
void* RemoteFreePool(RFPROCESS Process, void* HeapAddress) {
    return NULL;
}