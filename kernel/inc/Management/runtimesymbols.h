#pragma once
#include <krnltypes.h>
BOOL InitializeRuntimeSymbols();
void* KERNELAPI GetRuntimeSymbol(char* SymbolName);
void KERNELAPI CreateRuntimeSymbol(char* SymbolName, LPVOID SymbolReference);

KERNELSTATUS KERNELAPI _RT_SystemDebugPrint(LPWSTR Format, ...);