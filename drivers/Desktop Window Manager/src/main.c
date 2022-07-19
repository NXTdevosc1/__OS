#include <gdk.h>
#include <subsystem.h>
#include <kernelruntime.h>
#include <stdlib.h>
void testint(){
    KeSetDeathScreen(0x101, L"Test Death Screen By Interrupt", L"Hello...", L"https://nosssthing.com");
    
    while(1);
}

void __stdcall TestThread(){
    
    while(1);
}




GDKSTATUS GDKENTRY DriverEntry(DRIVER_HEADER* DriverHeader, LPCWSTR SystemPath)
{
    HTHREAD Thread = KeCreateThread(KeGetCurrentProcess(), 0, TestThread, 0, 0, 0);
    
    if(!Thread) testint();
    while(1);
    SystemDebugPrint(L"test");
    //for(UINT64 i = 0;i<0x200000;i++);
    //KeSetDeathScreen(0x101, L"Test Death Screen By Kernel", L"Hello...", L"https://nothing.com");
    //SetIrqGate(testint, 0);
    //KeCreateProcess(NULL, L"Driver Test", SUBSYSTEM_NATIVE, CREATE_PROCESS_KERNEL);
    while(1);
    //foofi();
    //SystemDebugPrint(L"Hello World");
    //DwmSubsystemInitialize(DriverHeader);
    DriverHeader->DriverTerminationProcedure = DriverTerminate;
    //DriverEntry(DriverHeader, SystemPath);
    while(1);
    return GDK_SUCCESS;
}

GDKSTATUS GDKAPI DriverTerminate(DRIVER_HEADER* DriverHeader){
    return GDK_SUCCESS;
}