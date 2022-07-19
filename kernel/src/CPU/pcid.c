#include <CPU/pcid.h>
#include <MemoryManagement.h>
UINT NumProcessContextIdentifiers = 0; // if == 0x1000 (function returns null and pcid gets disabled until it gets free)
BOOL ProcessContextIdSupported = FALSE;
BOOL ProcessContextIdEnabled = FALSE;
PCID_LIST PcIdList = { 0 };

BOOL ProcessContextIdAllocate(RFPROCESS Process) {
	if (!Process) return FALSE;
	if (!ProcessContextIdEnabled) return FALSE;
	if (NumProcessContextIdentifiers == MAX_PCID) {
		// disable until some pcids gets free
		ProcessContextIdDisable();
		ProcessContextIdEnabled = FALSE;
		return FALSE;
	}
	PCID_LIST* List = &PcIdList;
	for (UINT i = 0; i < MAX_PCID; i++) {
		if (!PcIdList.Tokens[i].Set) {
			PcIdList.Tokens[i].Set = TRUE;
			(UINT64)Process->PageMap |= (i + 1); // PCID
			NumProcessContextIdentifiers++;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ProcessContextIdFree(RFPROCESS Process) {
	if (!ProcessContextIdSupported || !Process || !((UINT64)Process->PageMap & 0xFFF)) return FALSE;
	UINT PcId = ((UINT64)Process->PageMap & 0xFFF) - 1;
	PcIdList.Tokens[PcId].Set = FALSE;
	if (ProcessContextIdSupported && !ProcessContextIdEnabled) // Lot of PCID's (We got a free PCID)
	{
		CheckProcessContextIdEnable();
		ProcessContextIdEnabled = TRUE;
	}
	NumProcessContextIdentifiers--;
	return TRUE;
}