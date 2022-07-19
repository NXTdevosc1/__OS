#include <sys/identify.h>
#include <mem.h>
#include <stddef.h>
#include <CPU/process.h>
#include <CPU/paging.h>
SYSIDENTIFY GlobalSysInformation = {
    1, 0, OS_BUILD_NAME
};
int SysIdentify(LPSYSIDENTIFY lpSysIdentify){
    SYS_RESTORE_CR3;
    memcpy(lpSysIdentify->build_name, GlobalSysInformation.build_name, sizeof(OS_BUILD_NAME));
    lpSysIdentify->major_version = GlobalSysInformation.major_version;
    lpSysIdentify->minor_version = GlobalSysInformation.minor_version;
    return 0;
}