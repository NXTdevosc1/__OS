#include <CPU/descriptor_tables.h>

#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <sysentry/sysentry.h>

void GlobalCpuDescriptorsInitialize(void* CpuBuffer)
{

	// load gdt
	struct GDT* CpuGdt = (struct GDT*)(CpuBuffer);
	struct TSS_ENTRY* CpuTSS = (struct TSS_ENTRY*)((UINT64)CpuBuffer + CPU_BUFFER_TSS_BASE);

	struct GDT_DESCRIPTOR* GdtDescriptor = (struct GDT_DESCRIPTOR*)((UINT64)CpuGdt + 0x1000);


	CpuGdt->kernel_code = (struct GDT_ENTRY){ 0, 0, 0, 0b10011010, 0, 0b1010, 0};
	CpuGdt->kernel_data = (struct GDT_ENTRY){ 0, 0, 0, 0b10010010, 0, 0b1000, 0};
	CpuGdt->user_code = (struct GDT_ENTRY){   0, 0, 0, 0b11111010, 0, 0b1010, 0};
	CpuGdt->user_data = (struct GDT_ENTRY){   0, 0, 0, 0b11110010, 0, 0b1000, 0};
	CpuGdt->tss = (struct TSS__LDT_DESCRIPTOR){sizeof(struct TSS_ENTRY), 0, 0, 0b10001001,0, 0b1000, 0, 0, 0};
	memcpy(&CpuGdt->SysenterSS, &CpuGdt->user_data, sizeof(struct GDT_ENTRY)); // Copy of User Data for Sysenter SS
	
	//CpuGdt->tss


	CpuTSS->rsp0 = (UINT64)CpuBuffer + CPU_BUFFER_IST1_BASE + TSS_IST1_BASE;
	CpuTSS->IOPB_offset = sizeof(struct TSS_ENTRY);
	CpuTSS->ist1 = CpuTSS->rsp0;
	CpuTSS->ist2 = (UINT64)CpuBuffer + CPU_BUFFER_IST2_BASE + TSS_IST2_BASE; // Task Scheduler RSP
	CpuTSS->ist3 = (UINT64)CpuBuffer + CPU_BUFFER_IST3_BASE + TSS_IST3_BASE;

	GdtDescriptor->size = sizeof(struct GDT) - 1;
	GdtDescriptor->offset = (UINT64)CpuGdt;


	SetTSS(&CpuGdt->tss, CpuTSS);
	LoadGDT(GdtDescriptor);
	FlushTSS(KERNEL_TSS_OFFSET); // loads kernel tss

}

void SetTSS(struct TSS__LDT_DESCRIPTOR* descriptor, struct TSS_ENTRY* tss) // sets tss entry and reloads gdt
{
	descriptor->base_low = (uint16_t)((uint64_t)tss);
	descriptor->base_mid = (uint8_t)((uint64_t)tss >> 16);
	descriptor->base_high = (uint8_t)((uint64_t)tss >> 24);
	descriptor->base_extended = (uint32_t)((uint64_t)tss >> 32);
}
