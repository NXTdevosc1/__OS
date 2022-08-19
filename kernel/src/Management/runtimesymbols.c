#include <CPU/process.h>
#include <Management/runtimesymbols.h>
#include <interrupt_manager/SOD.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <ipc/ipc.h>
#include <ipc/ipcserver.h>
#include <IO/pcie.h>
#include <CPU/paging.h>
#include <preos_renderer.h>
#include <cpu/cpu.h>
#include <cstr.h>
typedef struct _LINK_SYMBOL {
    char* SymbolName;
    void* SymbolReference;
} LINK_SYMBOL;

#define KERNEL_LINK_SYMBOLS_SIZE 120

LINK_SYMBOL KernelLinkSymbols[KERNEL_LINK_SYMBOLS_SIZE] = { 0 };

BOOL SymbolsCreated = FALSE;
// For Drivers

UINT64 SymbolIndex = 0;
// RT (Runtime)
BOOL KERNELAPI _RTMapPhysicalPages(RFPROCESS Process, void* VirtualAddress, void* PhysicalAddress, UINT64 NumPages, UINT64 Flags){
    return MapPhysicalPages(Process->PageMap, VirtualAddress, PhysicalAddress, NumPages, Flags);
}

BOOL KERNELAPI _RT_MapKernelMemory(void* PhysicalAddress, UINT64 NumPages, UINT64 Flags){
    return MapPhysicalPages(kproc->PageMap, PhysicalAddress, PhysicalAddress, NumPages, Flags);
}
UINT64 YOFF = 20;
UINT64 __DEBUGPRINT_MUTEX = 0;
UINT64 SDBGP_BiggestWidth = 0;

#define MAX_FORMAT_LENGTH 0x100

UINT16 _DEBUGPRINT_FORMATTED[0x1000] = {0};
char _DEBUGPRINT_FORMATNUMBER[0x100] = {0};

#define IS_LETTER(Char) ((Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z'))

KERNELSTATUS KERNELAPI SystemDebugPrint(LPWSTR Format, ...){
    if(!Format) return KERNEL_SERR_INVALID_PARAMETER;
    UINT8 Len = wstrlen(Format);
    if(!Len) return KERNEL_SERR_INVALID_PARAMETER;
    RFPROCESS Process = KeGetCurrentProcess();
    UINT RFLAGS = __getRFLAGS();
    __cli();
    // if(Process != SystemInterruptsProcess || Process != kproc) // May set a permanent spinlock on system interrupts
    //     __SpinLockSyncBitTestAndSet(&__DEBUGPRINT_MUTEX, 0);
    
    UINT   StackOff = 0;
    UINT8   FormatIndex = 0;
    // Calculate Result Length
    ZeroMemory(_DEBUGPRINT_FORMATTED, 0x100);
    for(UINT8 i = 0;i< Len;){
        if(Format[i] == L'%'){
            LPWSTR Next = &Format[i + 1];
            StackOff += 8;
            void* Value = (void*)((UINT64)&Format + StackOff);
            if(Next[0] == 'd' && !IS_LETTER(Next[1])){
                // write decimal number
                int Number = *(unsigned long long*)Value;
                itoa(Number, _DEBUGPRINT_FORMATNUMBER, RADIX_DECIMAL);
                for(UINT c = 0;_DEBUGPRINT_FORMATNUMBER[c];c++){
                    _DEBUGPRINT_FORMATTED[FormatIndex] = _DEBUGPRINT_FORMATNUMBER[c];
                    FormatIndex++;
                }
                i+=2;
            }else if(Next[0] == '%' && !IS_LETTER(Next[1]))
            {
                // write binary number
                unsigned long long Number = *(unsigned long long*)(Value);
                itoa(Number, _DEBUGPRINT_FORMATNUMBER, RADIX_BINARY);
                for(UINT c = 0;_DEBUGPRINT_FORMATNUMBER[c];c++){
                    _DEBUGPRINT_FORMATTED[FormatIndex] = _DEBUGPRINT_FORMATNUMBER[c];
                    FormatIndex++;
                }
                i+=2;
            }else if(Next[0] == 'x' && !IS_LETTER(Next[1]))
            {
                // write hexadecimal number
                unsigned long long Number = *(unsigned long long*)(Value);
                itoa(Number, _DEBUGPRINT_FORMATNUMBER, RADIX_HEXADECIMAL);
                for(UINT c = 0;_DEBUGPRINT_FORMATNUMBER[c];c++){
                    _DEBUGPRINT_FORMATTED[FormatIndex] = _DEBUGPRINT_FORMATNUMBER[c];
                    FormatIndex++;
                }
                i+=2;
            }else if(Next[0] == 'c' && !IS_LETTER(Next[1])){
                // Write Character
                if(*(unsigned char*)Value) {
                _DEBUGPRINT_FORMATTED[FormatIndex] = *(unsigned char*)Value;
                }else {
                    _DEBUGPRINT_FORMATTED[FormatIndex] = L' ';
                }
                FormatIndex++;
                i+=2;
            }else if(Next[0] == 's' && !IS_LETTER(Next[1])){
                // Write String
                unsigned char* Str = *(unsigned char**)Value;
                while(*Str){
                    if(FormatIndex >= MAX_FORMAT_LENGTH) break;
                    _DEBUGPRINT_FORMATTED[FormatIndex] = *Str;
                    Str++;
                    FormatIndex++;
                }
                i+=2;
            }else if(Next[0] == 'l' && Next[1] == 's' && !IS_LETTER(Next[2])){
                // Write unicode string
                unsigned short* Str = *(unsigned short**)Value;
                while(*Str){
                    if(FormatIndex >= MAX_FORMAT_LENGTH) break;
                    _DEBUGPRINT_FORMATTED[FormatIndex] = *Str;
                    Str++;
                    FormatIndex++;
                }
                i+=3;
            }else{
                _DEBUGPRINT_FORMATTED[FormatIndex] = Format[i];
                FormatIndex++;
                i++;
            }
        }else{
            _DEBUGPRINT_FORMATTED[FormatIndex] = Format[i];
            FormatIndex++;
            i++;
        }
    }

    // Format string

    LPWSTR ProcessName = L"Unnamed Process.";
    if(Process->ProcessName) ProcessName = Process->ProcessName;

    UINT8 PNAME_LEN = wstrlen(ProcessName);
    Len = wstrlen(_DEBUGPRINT_FORMATTED);

    // if(Len > 100) Len = 100;
    if(YOFF >= InitData.fb->VerticalResolution - 40){
        GP_draw_rect(20, 20, SDBGP_BiggestWidth, YOFF, 0);
        YOFF = 20;
        SDBGP_BiggestWidth = 0;
    }

    if(YOFF != 20){
        GP_draw_rect(5, YOFF - 16 + (8 - 1), 10, 2, 0);
    }
    GP_draw_rect(20, YOFF, (Len + PNAME_LEN + 3) << 3, 16, 0);
    if(( (Len + PNAME_LEN + 3) << 3) > SDBGP_BiggestWidth){
        SDBGP_BiggestWidth =  (Len + PNAME_LEN + 3) << 3;
    }
    Gp_draw_sf_textW(ProcessName, 0xE29910, 20, YOFF);
    GP_draw_sf_text(" : ", 0xE29910, 20 + (PNAME_LEN << 3), YOFF);
    for(UINT8 i = 0;i<Len;i++){
        GP_sf_put_char((char)_DEBUGPRINT_FORMATTED[i], 0xffffff, 20 + ((PNAME_LEN + 3 + i) << 3), YOFF);
    }
    GP_draw_rect(5, YOFF + (8 - 1), 10, 2, 0xffffff);
    YOFF+=16;
    // if(Process != SystemInterruptsProcess || Process != kproc)
    //     __BitRelease(&__DEBUGPRINT_MUTEX, 0);

    __setRFLAGS(RFLAGS);

    return KERNEL_SOK;
}

BOOL InitializeRuntimeSymbols(){
    if (SymbolsCreated) return FALSE;
    // kernelruntime.h

    // CreateRuntimeSymbol("SystemDebugPrint", SystemDebugPrint);
    
    // CreateRuntimeSymbol("KeCreateProcess", KeCreateProcess);
    // CreateRuntimeSymbol("KeCreateThread", KeCreateThread);
    // CreateRuntimeSymbol("KeTaskSchedulerEnable", TaskSchedulerEnable);
    // CreateRuntimeSymbol("KeTaskSchedulerDisable", TaskSchedulerDisable);
    // CreateRuntimeSymbol("KeGetRuntimeSymbol", GetRuntimeSymbol);
    // CreateRuntimeSymbol("KeSetPriorityClass", SetPriorityClass);
    // CreateRuntimeSymbol("KeSetThreadPriority", SetThreadPriority);
    // CreateRuntimeSymbol("KeGetCurrentProcess", GetCurrentProcess);
    // CreateRuntimeSymbol("KeGetProcessById", GetProcessById);
    // CreateRuntimeSymbol("KeGetCurrentThread", GetCurrentThread);
    // CreateRuntimeSymbol("KeGetCurrentProcessId", GetCurrentProcessId);
    // CreateRuntimeSymbol("KeGetCurrentThreadId", GetCurrentThreadId);
    // CreateRuntimeSymbol("KeSetDeathScreen", KeSetDeathScreen);
    // CreateRuntimeSymbol("malloc", CurrentProcessMalloc);
    // CreateRuntimeSymbol("KeExtendedAlloc", ExtendedMemoryAlloc);
    // CreateRuntimeSymbol("free", CurrentProcessFree);
    // CreateRuntimeSymbol("MapPhysicalPages", _RTMapPhysicalPages);
    // CreateRuntimeSymbol("KeMapMemory", _RT_MapKernelMemory);
    // CreateRuntimeSymbol("KeResumeThread", ResumeThread);
    // CreateRuntimeSymbol("KeSuspendThread", SuspendThread);
    // CreateRuntimeSymbol("KeControlIrq", KeControlIrq);
    // CreateRuntimeSymbol("KeReleaseIrq", KeReleaseIrq);
    // CreateRuntimeSymbol("Sleep", Sleep);
    // CreateRuntimeSymbol("MicroSleep", MicroSleep);
    // CreateRuntimeSymbol("SetInterruptService", SetInterruptService);
    // CreateRuntimeSymbol("IoWait", IoWait);
    // CreateRuntimeSymbol("IoFinish", IoFinish);
    
    // // sysipc.h
    // CreateRuntimeSymbol("IpcClientCreate", IpcClientCreate);
    // CreateRuntimeSymbol("IpcIsValidClient", IpcIsValidClient);
    // CreateRuntimeSymbol("IpcClientDestroy", IpcClientDestroy);

    // CreateRuntimeSymbol("IpcSendMessage", IpcSendMessage);
    // CreateRuntimeSymbol("IpcSetCallStatus", IpcSetStatus);
    // CreateRuntimeSymbol("IpcMessageDispatch", IpcMessageDispatch);
    // CreateRuntimeSymbol("IpcPostThreadMessage", IpcPostThreadMessage);
    // CreateRuntimeSymbol("IpcGetMessage", IpcGetMessage);
    // CreateRuntimeSymbol("IpcGetThreadClient", IpcGetThreadClient);
    // // ipcserver.h
    // CreateRuntimeSymbol("IpcServerCreate", IpcServerCreate);
    // CreateRuntimeSymbol("IpcServerDestroy", IpcServerDestroy);
    // CreateRuntimeSymbol("IpcServerConnect", IpcServerConnect);
    // CreateRuntimeSymbol("IpcServerDisconnect", IpcServerDisconnect);
    // CreateRuntimeSymbol("IpcSendToServer", IpcSendToServer);
    // CreateRuntimeSymbol("IpcServerSend", IpcServerSend);
    // CreateRuntimeSymbol("IpcServerReceive", IpcServerReceive);
    // CreateRuntimeSymbol("IpcBroadcast", IpcBroadcast);
    // CreateRuntimeSymbol("IpcServerMessageDispatch", IpcServerMessageDispatch);
    // CreateRuntimeSymbol("IpcGetServer", IpcGetServer);
    // CreateRuntimeSymbol("IpcIsValidServer", isValidServer);
    // CreateRuntimeSymbol("IpcCheckClientConnection", isConnectedClient);


    // // pciexpressapi
    // CreateRuntimeSymbol("CheckPciExpress", GetPcieCompatibility);
    // CreateRuntimeSymbol("PciExpressConfigurationRead", PcieConfigurationRead);
    // CreateRuntimeSymbol("PciExpressEnumerateDevices", PcieEnumerateDevices);
    // CreateRuntimeSymbol("PciExpressFindDevice", PcieFindDevice);

    // // Runtime Variables

    // CreateRuntimeSymbol(".$_BasicGraphicsOutputDescriptor", InitData.fb);

    
    SymbolsCreated = TRUE;
    return TRUE;
}


void KERNELAPI CreateRuntimeSymbol(char* SymbolName, LPVOID SymbolReference) {
    if (SymbolIndex >= KERNEL_LINK_SYMBOLS_SIZE) SOD(SOD_DRIVER_ERROR, "Runtime Symbol Count Exceded : Check runtimesymbols.h/.c");
    KernelLinkSymbols[SymbolIndex].SymbolName = SymbolName;
    KernelLinkSymbols[SymbolIndex].SymbolReference = SymbolReference;
    SymbolIndex++;
}

LPVOID KERNELAPI GetRuntimeSymbol(char* SymbolName) {
    // Check if they are not initialized

    for (UINT64 i = 0; i < KERNEL_LINK_SYMBOLS_SIZE; i++) {
        if (strcmp(KernelLinkSymbols[i].SymbolName, SymbolName)) {
            return KernelLinkSymbols[i].SymbolReference;
        }
    }

    return NULL;
}
