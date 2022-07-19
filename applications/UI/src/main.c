#include <stdlib.h>
#include <user64.h>

int main(){
    char test[120] = {0};
    while(1);
    SysDebugPrint("Hello World");
    UINT64 ProcessId = GetCurrentProcessId();
    UINT64 ThreadId = GetCurrentThreadId();

    SysDebugPrint("Process Id :");
    itoa(ProcessId, test, RADIX_DECIMAL);
    SysDebugPrint(test);
    SysDebugPrint("Thread Id :");
    itoa(ThreadId, test, RADIX_DECIMAL);
    SysDebugPrint(test);
    
    while(1);
}