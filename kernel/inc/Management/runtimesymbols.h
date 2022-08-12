#pragma once
#include <krnltypes.h>
BOOL InitializeRuntimeSymbols();
LPVOID KEXPORT KERNELAPI GetRuntimeSymbol(char* SymbolName);
void KERNELAPI CreateRuntimeSymbol(char* SymbolName, LPVOID SymbolReference);

KERNELSTATUS KEXPORT KERNELAPI SystemDebugPrint(LPWSTR Format, ...);