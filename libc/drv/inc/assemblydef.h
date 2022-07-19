#pragma once
#include <kerneltypes.h>

#ifndef DDKIMPORT
#define DDKIMPORT __declspec(dllimport)
#endif

#ifndef __DLL_EXPORTS


DDKIMPORT void __cli();
DDKIMPORT void __sti();
DDKIMPORT void __hlt();

DDKIMPORT unsigned long long __getRFLAGS();
DDKIMPORT void __setRFLAGS(unsigned long long RFlags);
DDKIMPORT void OutPortB(unsigned short Port, unsigned char Value);
DDKIMPORT void OutPortW(unsigned short Port, unsigned short Value);
DDKIMPORT void OutPort(unsigned short Port, unsigned int Value);

DDKIMPORT unsigned char InPortB(unsigned short Port);
DDKIMPORT unsigned short InPortW(unsigned short Port);
DDKIMPORT unsigned int InPort(unsigned short Port);


DDKIMPORT unsigned long long __rdtsc();

DDKIMPORT void __wbinvd(); // write back invalidate cache

DDKIMPORT void __repstos(void* Address, unsigned char Value, unsigned long long Count);
DDKIMPORT void __repstos16(void* Address, unsigned short Value, unsigned long long Count);
DDKIMPORT void __repstos32(void* Address, unsigned int Value, unsigned long long Count);
DDKIMPORT void __repstos64(void* Address, unsigned long long Value, unsigned long long Count);

DDKIMPORT void __SpinLockSyncBitTestAndSet(void* Address, UINT16 BitOffset);
DDKIMPORT BOOL __SyncBitTestAndSet(void* Address, UINT16 BitOffset); // return 1 if success, 0 if fail
// Previous bit content is storred in carry flag (__getRFLAGS()) to test content of CF
DDKIMPORT void __BitRelease(void* Address, UINT16 BitOffset); // Does not need synchronization

DDKIMPORT void __SyncIncrement64(UINT64* Address);
DDKIMPORT void __SyncIncrement32(UINT32* Address);
DDKIMPORT void __SyncIncrement16(UINT16* Address);
DDKIMPORT void __SyncIncrement8(UINT8* Address);

DDKIMPORT void __SyncDecrement64(UINT64* Address);
DDKIMPORT void __SyncDecrement32(UINT32* Address);
DDKIMPORT void __SyncDecrement16(UINT16* Address);
DDKIMPORT void __SyncDecrement8(UINT8* Address);

#else

DDKIMPORT void __cli();
DDKIMPORT void __sti();
DDKIMPORT void __hlt();



#endif