#pragma once
#include <ddk.h>
#ifndef __DLL_EXPORTS
#ifndef DDKIMPORT
#define DDKIMPORT __declspec(dllimport)
#endif

DDKIMPORT void __setCR3(unsigned long long CR3);
DDKIMPORT void __SyncOr(void* Value, UINT BitOffset);
#else

#endif