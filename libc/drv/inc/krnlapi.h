#pragma once

#ifndef __DLL_EXPORTS
#define KERNELAPI __declspec(dllimport) __cdecl

#else
#define KERNELAPI __declspec(dllexport) __cdecl

#endif

#define __KERNELAPI __cdecl