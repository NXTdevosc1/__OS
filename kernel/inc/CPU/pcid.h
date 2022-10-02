#pragma once
#include <krnltypes.h>
#include <CPU/process_defs.h>
typedef struct _PCID_LIST PCID_LIST;

UINT NumProcessContextIdentifiers; // if == 0x1000 (function returns null and pcid gets disabled until it gets free)
BOOL ProcessContextIdSupported;
BOOL ProcessContextIdEnabled;

#define MAX_PCID 0xFFF

typedef struct _PCID_TOKEN {
	BOOL Set;
} PCID_TOKEN, *RFPCID_TOKEN;

typedef struct _PCID_LIST{
	PCID_TOKEN Tokens[MAX_PCID];
} PCID_LIST;

BOOL ProcessContextIdAllocate(RFPROCESS Process);
BOOL ProcessContextIdFree(RFPROCESS Process);

// returns true if pcid is supported
extern BOOL CheckProcessContextIdEnable(void);
extern void ProcessContextIdDisable(void);
extern BOOL CheckInvPcidSupport(void);