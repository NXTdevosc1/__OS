#pragma once
#include <krnltypes.h>
// #define ___KERNEL_DEBUG___// Temporarely set


#define DEBUG_MAX_LOG_LENGTH 150
#define DEBUG_MAX_LOG_ENTRIES 20
#define DEBUG_MAX_INDEX (DEBUG_MAX_LOG_ENTRIES - 1)


#pragma section("_KRNLDBG")

typedef struct _DEBUG_TABLE {
	UINT64 LogAllocateIndex;
	UINT64 TotalLogCount;
	UINT64 ControlBit; // Set by locking
	char   LogEntries[DEBUG_MAX_LOG_ENTRIES][DEBUG_MAX_LOG_LENGTH + 1];
	char 	Reserved[0x1000];
} DEBUG_TABLE;

void DebugWrite(const char* Text);
char* DebugRead(UINT Index);

DEBUG_TABLE KernelDebugTable;

