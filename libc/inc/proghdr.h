#pragma once
#ifdef __NODECL
#define _DECL
#else
#ifdef __DLLEXPORTS
#define _DECL __declspec(dllexport)
#include <syscall.h>
#else
#define _DECL __declspec(dllimport)
#endif
#endif
