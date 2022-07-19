#pragma once
#include <stdint.h>
#define OS_BUILD_NAME "IN DEV OS"
typedef struct _SYSIDENTIFY{
    uint16_t major_version;
    uint16_t minor_version;
    char build_name[20];
} SYSIDENTIFY, *LPSYSIDENTIFY;
int SysIdentify(LPSYSIDENTIFY lpSysIdentify);
extern SYSIDENTIFY GlobalSysInformation;