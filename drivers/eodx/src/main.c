#include <gdk.h>
#include <kernelruntime.h>
#define __EODX
#include <eodxsys.h>
#include <adapter.h>
#include <sysipc.h>
#include <ipcserver.h>
#include <stdlib.h>
GRAPHICS_DISPLAY_CONFIGURATION gdc = { 0 };
DRIVER_AUTHENTICATION_TABLE GlobalAuthenticationDescriptor = {0};

GDKSTATUS GDKENTRY DriverEntry(RFDRIVER Driver){
    
    RFTHREAD Thread = KeGetCurrentThread();
    RFCLIENT Client = IpcGetThreadClient(Thread);
    RFSERVER KernelServer = IpcServerConnect(
        IPC_MAKEADDRESS(0, 0, 0, 1),
        Client,
        NULL
    );
    if(!KernelServer) KeSetDeathScreen(0, L"Enhanced & Optimized Direct Graphics (EODX) Driver Error.", L"Failed to connect with kernel servers.", OS_SUPPORT_LNKW);
    MSG Message = {0};
    KeSuspendThread(Thread);
    for(UINT64 i = 0;;i++){
        Message.Message = i;
        if(KERNEL_ERROR(
            IpcSendToServer(
                Client, KernelServer, TRUE, &Message
            )
        )) KeSetDeathScreen(0, L"Enhanced & Optimized Direct Graphics (EODX) Driver Error.", L"Failed to send message to kernel servers.", OS_SUPPORT_LNKW);
    }
    // RFTHREAD Thd_FrameUpdate = KeCreateThread(KeGetCurrentProcess(), 0x5000, FrameUpdateThread, 0, 0, 0);
    // if(!Thd_FrameUpdate){
    //     KeSetDeathScreen(0, L"Enhanced & Optimized Direct Graphics (EODX) Driver Error.", L"Failed to initialize frame update thread.", OS_SUPPORT_LNKW);
    // }
    // if(!InitializeBasicDisplayAdapter(&gdc))
    // {
    //     KeSetDeathScreen(0, L"Enhanced & Optimized Direct Graphics (EODX) Driver Error.", L"Cannot initialize basic display adapter.", OS_SUPPORT_LNKW);
    // }



    while(1);
}