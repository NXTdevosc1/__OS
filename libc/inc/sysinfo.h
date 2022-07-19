#pragma once
#include <usertypes.h>
#include <proghdr.h>

typedef struct _SYSIDENTIFY{
    UINT16 major_version;
    UINT16 minor_version;
    char build_name[20];
} SYSIDENTIFY, *LPSYSIDENTIFY;

typedef struct _MEMORYSTATUS{
    DWORD MemoryLoad;
    QWORD VirtualMemoryLoad;
    QWORD TotalPhys;
    QWORD AvailaiblePhys;
    QWORD TotalPageFile;
    QWORD AvailaiblePageFile;
    QWORD TotalVirtual;
    QWORD AvailaibleVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;



_DECL WSTATUS SysIdentify(LPSYSIDENTIFY LpSysIdentify);
_DECL BOOL GlobalMemoryStatus(LPMEMORYSTATUS lpMemoryStatus);

_DECL float GetThreadCpuTime(HTHREAD Thread);
_DECL float GetProcessCpuTime(HPROCESS Process);
_DECL float GetTotalCpuTime();
_DECL float GetIdleCpuTime();

_DECL UINT64 GetTotalCpuTimerClocks();
_DECL UINT64 GetThreadCpuTimerClocks();
_DECL UINT64 GetProcessCpuTimerClocks();
_DECL UINT64 GetIdleCpuTimerClocks();