#pragma once
#include <proghdr.h>
#include <usertypes.h>
#include <stdlib.h>
#include <sysinfo.h>
#ifndef __LEAN_AND_MEAN__
#include <mem.h>
#endif

#define ZeroMemory(ptr, sz) memset(ptr, 0, sz)
#define ZeroMemoryA(ptr) memset(ptr, 0, sizeof(*ptr)) // Zero Memory Auto



_DECL void __cdecl SysDebugPrint(LPCSTR text);
_DECL HPROCESS __cdecl GetCurrentProcess();
_DECL HTHREAD __cdecl GetCurrentThread();

_DECL UINT64 __cdecl GetCurrentProcessId();
_DECL UINT64 __cdecl GetCurrentThreadId();