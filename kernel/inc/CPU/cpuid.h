#pragma once


typedef struct _CPUID_INFO{
    unsigned int eax, ebx, ecx, edx;
} CPUID_INFO;

void __cpuid(CPUID_INFO* CpuInfo, int FunctionId);


// CPUID [EAX = 1] ECX:

#define CPUID1_ECX_SSE3 1
#define CPUID1_ECX_PCLMULDQ (1 << 1)
#define CPUID1_ECX_DTES64 (1 << 2)
#define CPUID1_ECX_MONITOR (1 << 3)
#define CPUID1_ECX_DSCPL (1 << 4)
#define CPUID1_ECX_VMX (1 << 5)
#define CPUID1_ECX_SMX (1 << 6)
#define CPUID1_ECX_EIST (1 << 7) // Enhanced Intel SpeedStep Technology
#define CPUID1_ECX_TM2 (1 << 8)
#define CPUID1_ECX_SSSE3 (1 << 9)
#define CPUID1_ECX_CNXTID (1 << 10)
#define CPUID1_ECX_FMA (1<<12)
#define CPUID1_ECX_CX16 (1<<13)
#define CPUID1_ECX_XTPR (1<<14)
#define CPUID1_ECX_PDCM (1<<15)
#define CPUID1_ECX_PCID (1 << 17)
#define CPUID1_ECX_DCA (1 << 18)
#define CPUID1_ECX_SSE4_1 (1 << 19)
#define CPUID1_ECX_SSE4_2 (1 << 20)
#define CPUID1_ECX_X2APIC (1 << 21)
#define CPUID1_ECX_MOVBE (1 << 22)
#define CPUID1_ECX_POPCNT (1 << 23)
#define CPUID1_ECX_APICTMR_TSC_DEADLINE (1 << 24)
#define CPUID1_ECX_AES (1 << 25)
#define CPUID1_ECX_XSAVE (1 << 26)
#define CPUID1_ECX_OSXSAVE (1<<27)
#define CPUID1_ECX_AVX (1 << 28)
#define CPUID1_ECX_F16C (1 << 29)
#define CPUID1_ECX_RDRAND (1 << 30)

// CPUID [EAX = 1] EDX:

#define CPUID1_EDX_FPU 1
#define CPUID1_EDX_VME (1 << 1)
#define CPUID1_EDX_DE (1 << 2)
#define CPUID1_EDX_PSE (1 << 3)
#define CPUID1_EDX_TSC (1 << 4)
#define CPUID1_EDX_MSR (1 << 5)
#define CPUID1_EDX_PAE (1 << 6)
#define CPUID1_EDX_MCE (1 << 7)
#define CPUID1_EDX_CX8 (1 << 8)
#define CPUID1_EDX_APIC (1 << 9)


#define CPUID1_EDX_FAST_SYSCALL (1 << 11) // Support For Sysenter/Sysexit Instructions



#define CPUID1_EDX_MTRR (1 << 12)
#define CPUID1_EDX_PGE (1 << 13)
#define CPUID1_EDX_MCA (1 << 14)
#define CPUID1_EDX_CMOV (1 << 15)
#define CPUID1_EDX_PAT (1 << 16)
#define CPUID1_EDX_PSE36 (1 << 17)
#define CPUID1_EDX_PSN (1 << 18)
#define CPUID1_EDX_CLFLUSH (1 << 19)
#define CPUID1_EDX_DS (1 << 21)
#define CPUID1_EDX_ACPI (1 << 22)
#define CPUID1_EDX_MMX (1 << 23)
#define CPUID1_EDX_FXSR (1 << 24)
#define CPUID1_EDX_SSE (1 << 25)
#define CPUID1_EDX_SSE2 (1 << 26)
#define CPUID1_EDX_SELF_SNOOP (1 << 27)
#define CPUID1_EDX_MUTLI_THREADING (1 << 28)
#define CPUID1_EDX_THERMAL_MONITOR (1 << 29)
#define CPUID1_EDX_PENDING_BREAK_ENABLE (1 << 31)


