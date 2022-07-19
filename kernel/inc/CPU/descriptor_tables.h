#pragma once
#include <stdint.h>
#define KERNEL_TSS_OFFSET 0x38
#define USER_SEGMENT_BASE 0x18

#define SYSENTER_SS 0x30

#define TSS_IST1_BASE 0x1C000
#define TSS_IST2_BASE 0xC000
#define TSS_IST3_BASE 0x2C000

#define TSS_IST1_NUMPAGES 0x20
#define TSS_IST2_NUMPAGES 0x10
#define TSS_IST3_NUMPAGES 0x30


#define CPU_BUFFER_TSS_BASE 0x2000
#define CPU_BUFFER_IST1_BASE 0x3000
#define CPU_BUFFER_IST2_BASE (CPU_BUFFER_IST1_BASE + (TSS_IST1_NUMPAGES << 12))
#define CPU_BUFFER_IST3_BASE (CPU_BUFFER_IST2_BASE + (TSS_IST2_NUMPAGES << 12))
#pragma pack(push, 1)
struct GDT_ENTRY{
		uint16_t limit_low;
		uint16_t base_low;
		uint8_t base_mid;
		uint8_t access;
		uint8_t limit_high : 4;
		uint8_t flags : 4;
		uint8_t base_high;
};
struct GDT_DESCRIPTOR{
	uint16_t size;
	uint64_t offset;
};
struct TSS__LDT_DESCRIPTOR{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t limit_high : 4;
	uint8_t flags : 4;
	uint8_t base_high;
	uint32_t base_extended;
	uint32_t reserved;
};

struct TSS_ENTRY{
	uint32_t reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t IOPB_offset;
};

struct GDT{
	struct GDT_ENTRY null0;
	struct GDT_ENTRY kernel_code;
	struct GDT_ENTRY kernel_data;
	struct GDT_ENTRY null1; // for sysret get base of null1, ss = null1+8, cs = null1+16
	struct GDT_ENTRY user_data;
	struct GDT_ENTRY user_code;
	struct GDT_ENTRY SysenterSS; // a copy of user_data
    struct TSS__LDT_DESCRIPTOR tss; // worth 16 bytes
	// struct TSS__LDT_DESCRIPTOR ldt;
};
#pragma pack(pop)
void GlobalCpuDescriptorsInitialize(void* CpuBuffer);
extern void LoadGDT(struct GDT_DESCRIPTOR* descriptor);
extern void FlushTSS(uint16_t off);
void SetTSS(struct TSS__LDT_DESCRIPTOR* descriptor, struct TSS_ENTRY* tss);



extern struct GDT gdt;
extern struct GDT_DESCRIPTOR gdtr;
extern struct TSS_ENTRY KERNEL_TSS;
extern long long SysenterRSP[0x400];