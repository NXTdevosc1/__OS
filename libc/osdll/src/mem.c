#define __DLLEXPORTS

#include <mem.h>

_DECL void* __cdecl malloc(size_t size){
    __SyscallNumber(SYSCALL_MALLOC);
    return (void*)EnterSystem((_SCA)size);
}
_DECL void* __cdecl free(void* Heap){
    __SyscallNumber(SYSCALL_FREE);
    return (void*)EnterSystem((_SCA)Heap);
}
_DECL void* __cdecl ExtendedAlloc(unsigned long long Align, size_t Size){
    __SyscallNumber(SYSCALL_EXTENDED_ALLOC);
    return (void*)EnterSystem((_SCA)Align, Size);
}