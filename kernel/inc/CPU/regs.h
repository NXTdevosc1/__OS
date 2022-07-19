#pragma once
#include <krnltypes.h>

#pragma pack(push, 1)

struct CPU_REGISTERS_X86_64{
    UINT64 rax; // accumulator
    UINT64 rbx; // base
    UINT64 rcx; // counter
    UINT64 rdx; // data
    UINT64 rdi;
    UINT64 rsi;
    UINT64 r8; // r(x) general purpose
    UINT64 r9;


    UINT64 rip;
    UINT64 cs;
    UINT64 rflags; // cpu flags
    UINT64 rsp; // stack pointer
    // UINT64
    UINT16 ds;
    UINT16 ss;
    UINT16 gs;
    UINT16 fs;

    UINT64 es;
    UINT64 cr3;
    LPVOID Xsave;


    UINT64 r10;
    UINT64 r11;
    UINT64 r12;
    UINT64 r13;
    UINT64 r14;
    UINT64 r15;
    UINT64 rbp; // base pointer
    UINT64 Reserved;
};

#pragma pack(pop)