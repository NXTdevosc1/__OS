#pragma once
#include <krnltypes.h>

/*
TO KNOW HOW EXACTLY PAGING WORKS

CR3 = PAGE ALLIGNED ADDR TO A PML4 ENTRY ARRAY OF 512 ENTRIES

*/

enum PAGEMAP_FLAGS {
    PM_PRESENT = 1,
    PM_USER = 2,
    PM_READWRITE = 4,
    PM_MAP = PM_PRESENT | PM_READWRITE,
    PM_UMAP = PM_MAP | PM_USER,
    PM_UNMAP = 0,
    PM_NX = 8, // No execute Enable
    PM_GLOBAL = 0x10,
    PM_CACHE_DISABLE = 0x20,
    PM_WRITE_THROUGH = 0x40, // Write in the cache and also in memory,
    PM_NO_CR3_RELOAD = 0x80, // for e.g. used on the kernel initialization to not reload CR3 or paging structures
    PM_NO_TLB_INVALIDATION = 0x100, // also used on kernel initialization
    PM_LARGE_PAGES         = 0x200 // Map to 2MB Pages (Count value specifies number of 2mb blocks)
};

typedef struct _PAGE_TABLE_ENTRY {
    UINT64 Present : 1;
    UINT64 ReadWrite : 1;
    UINT64 UserSupervisor : 1;
    UINT64 PageLevelWriteThrough : 1;
    UINT64 PageLevelCacheDisable : 1;
    UINT64 Accessed : 1;
    UINT64 Dirty : 1;
    UINT64 Size : 1;
    UINT64 Global : 1;
    UINT64 Ignored0 : 3;
    UINT64 PhysicalAddr : 36;
    UINT64 Ignored1 : 15;
    UINT64 ExecuteDisable : 1;
} PTBLENTRY, *RFPAGEMAP;

