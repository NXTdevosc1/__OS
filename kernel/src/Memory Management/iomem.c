#include <MemoryManagement.h>

LPVOID KEXPORT KERNELAPI AllocateIoMemory(_IN LPVOID PhysicalAddress, _IN UINT64 NumPages, _IN UINT Flags) {
    return NULL;
}
BOOL KEXPORT KERNELAPI FreeIoMemory(_IN LPVOID IoMemory) {
    return FALSE;
}