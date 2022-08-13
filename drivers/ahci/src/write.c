#include <ddk.h>
#include <ahci.h>

DDKSTATUS AhciSataWrite(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}



DDKSTATUS AhciSatapiWrite(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer) {
    return KERNEL_SERR;
}