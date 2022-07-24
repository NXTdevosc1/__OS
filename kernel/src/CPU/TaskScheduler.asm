
[BITS 64]



global ScheduleTask
global SkipTaskSchedule ; Skips the current task and schedule (does not wait for clocks to finish)

global EnterThread ; Used on interrupt handlers

extern KeGlobalCR3
extern Pmgrt
global __InterruptCheckHalt
extern CpuManagementTable
extern GlobalThreadPreemptionPriorities

extern ApicTimerBaseQuantum
extern ApicTimerClockCounter
extern TimerIncrementerCpuId

; DEFINES
	APIC	 equ 0xFEE00000
	APIC_ID		equ 0x20
	APIC_EOI equ APIC + 0x0B0
	APIC_TIMER_INITIAL_COUNT equ APIC + 0x380
	
	BASE_TIMER_TICKS equ 1800
	APIC_PROCESSORID equ APIC + 0x20
	TPPL_MAX equ 100
	KRNL_DS equ 0x10
	PREEMPTION_STATE_MAX equ 56 ;7 * 8 MAX + 1, When this value is reached, Preemption State Resets
	
	GLOBAL_USER_PMGRT_VIRTUAL_ADDRESS equ 0x20000
	VIRTUAL_PMGRT_TSD equ GLOBAL_USER_PMGRT_VIRTUAL_ADDRESS + 32
	VIRTUAL_PMGRT_RAX equ VIRTUAL_PMGRT_TSD
	VIRTUAL_PMGRT_RBX equ VIRTUAL_PMGRT_TSD + 8
; PRIORITY PTR OFFSETS
	PRIORITY_CLASS_INDEX_IDLE equ 0
	PRIORITY_CLASS_INDEX_LOW equ 8
	PRIORITY_CLASS_INDEX_BELOW_NORMAL equ 0x10
	PRIORITY_CLASS_INDEX_NORMAL equ 0x18
	PRIORITY_CLASS_INDEX_ABOVE_NORMAL equ 0x20
	PRIORITY_CLASS_INDEX_HIGH equ 0x28
	PRIORITY_CLASS_INDEX_REALTIME equ 0x30

; PMGRT OFFSETS
	SCHEDULER_ENABLE	equ 0
	CPU_TIME_CALCULATION			equ 4
	CPU_COUNT		equ 8
	IDLE_CPU_TIME			equ 16
	SCHEDULER_CPU_TIME		equ 24
	;TASK_SCHEDULER_DATA equ 32
	THREAD_LIST_PTR_COUNT equ 104
	PRIORITY_CLASSES equ 160
	LP_INITIAL_PRIORITY_CLASSES equ 216
	INITIAL_PRIORITY_CLASSES equ 272


; CPU MGMT TABLE
	CPM_INITIALIZED equ 0
	CPM_PROCESSORID equ 4
	CPM_TOTAL_THREADS EQU 8
	CPM_NUM_READY_THREADS EQU 0x40
	CPM_THREAD_QUEUES EQU 0x78
	CPM_CURRENT_THREAD EQU 0xB0
	CPM_SELECTED_THREAD EQU 0xB8
	CPM_TOTAL_CLOCKS EQU 0xC0 ; 128 Bit value
	CPM_READY_ON_CLOCK EQU 0xD0
	CPM_HIGHEST_PRIORITY_THREAD EQU 0x108
	CPM_SYSTEM_IDLE_THREAD EQU 0x140
	CPM_SYSTEM_INTERRUPTS_THREAD EQU 0x148
	CPM_FORCE_NEXT_THREAD EQU 0x150
	CPM_CURRENT_CPU_TIME EQU 0x158
	CPM_ESTIMATED_CPU_TIME EQU 0x160
	CPM_IRQ_CONTROL_TABLE EQU 0x168

; THREAD_WAITING_QUEUE

THREAD_WAITING_QUEUE_NEXT equ 0x1000

; PROCESS THREAD TABLE
	TH_STATE equ 0
	TH_THREAD_ID equ 0x08
	TH_PROCESS equ 0x10
	TH_PROCESSORID equ 0x18
	TH_THREAD_PRIORITY equ 0x1C
	TH_TIME_BURST equ 0x20
	TH_WAITING_QUEUE_INDEX equ 0x24
	TH_WAITING_QUEUE equ 0x28
	TH_SCHEDULING_QUANTUM equ 0x30
	TH_READY_AT equ 0x34 ; 128 Bit Value
	TH_CONTROL_MUTEX equ 0x44
	TH_EXIT_CODE equ 0x4C
	TH_REGISTERS equ 0x54

	TH_REGISTER_SIZE equ 0xC0
	TH_STACK equ 0x114
	TH_CPU_TIME equ 0x11C
	TH_VIRTUAL_STACK_POINTER equ 0x124
	TH_CLIENT equ 0x12C
	TH_REMOVE_IOWAIT_AFTER equ 0x134
	TH_ATTEMPT_REMOVE_IOWAIT equ 0x138
	TH_REMAINING_CLOCKS equ 0x13C
	TH_SLEEP_UNTIL equ 0x140 ; 128 Bit Value
	TH_LAST_CALCULATION_TIME equ 0x150 ; 128 Bit Value


; REGISTERS (Packs are 64 Byte width (1 ZMM Register) )

_RAX equ TH_REGISTERS
_RBX equ TH_REGISTERS + 8
_RCX equ TH_REGISTERS + 0x10
_RDX equ TH_REGISTERS + 0x18
_RDI equ TH_REGISTERS + 0x20
_RSI equ TH_REGISTERS + 0x28
_R8 equ TH_REGISTERS + 0x30
_R9 equ TH_REGISTERS + 0x38
_RIP equ TH_REGISTERS + 0x40
_CS equ TH_REGISTERS + 0x48
_RFLAGS equ TH_REGISTERS + 0x50
_RSP equ TH_REGISTERS + 0x58

; PACK OF UINT16 (UINT64)(ds,ss,gs,fs)
_DS equ TH_REGISTERS + 0x60
_SS equ TH_REGISTERS + 0x62
_GS equ TH_REGISTERS + 0x64
_FS equ TH_REGISTERS + 0x66

_ES equ TH_REGISTERS + 0x68
_CR3 equ TH_REGISTERS + 0x70
_XSAVE equ TH_REGISTERS + 0x78
_R10 equ TH_REGISTERS + 0x80
_R11 equ TH_REGISTERS + 0x88
_R12 equ TH_REGISTERS + 0x90
_R13 equ TH_REGISTERS + 0x98
_R14 equ TH_REGISTERS + 0xA0
_R15 equ TH_REGISTERS + 0xA8
_RBP equ TH_REGISTERS + 0xB0


; SEGS : means 16 bit ds,ss,gs,fs
; RESV : Reserved

; MM16 (XMM [SSE])

; Pack 0
__MM16_RAXRBX equ TH_REGISTERS
__MM16_RCXRDX equ TH_REGISTERS + 0x10
__MM16_RDIRSI equ TH_REGISTERS + 0x20
__MM16_R8R9 equ TH_REGISTERS + 0x30

; Pack 1
__MM16_RIPCS equ TH_REGISTERS + 0x40
__MM16_RFLAGSRSP equ TH_REGISTERS + 0x50
__MM16_SEGSES equ TH_REGISTERS + 0x60
__MM16_CR3XSAVE equ TH_REGISTERS + 0x70

; Pack 2
__MM16_R10R11 equ TH_REGISTERS + 0x80
__MM16_R12R13 equ TH_REGISTERS + 0x90
__MM16_R14R15 equ TH_REGISTERS + 0xA0
__MM16_RBPRESV equ TH_REGISTERS + 0xB0

; MM32 (YMM [AVX])

; Pack 0
__MM32_RAXRBXRCXRDX equ TH_REGISTERS
__MM32_RDIRSIR8R9 equ TH_REGISTERS + 0x20

; Pack 1
__MM32_RIPCSRFLAGSRSP equ TH_REGISTERS + 0x40
__MM32_SEGSESCR3XSAVE equ TH_REGISTERS + 0x60

; Pack 2

__MM32_R10R11R12R13 equ TH_REGISTERS + 0x80
__MM32_R14R15RBPRESV equ TH_REGISTERS + 0xA0

; MM64 (ZMM (AVX512))
__MM64_RAXRBXRCXRDXRDIRSIR8R9 equ TH_REGISTERS
__MM64_RIPCSRFLAGSRSPSEGSESCR3XSAVE equ TH_REGISTERS + 0x40
__MM64_R10R11R12R13R14R15RBPRESV equ TH_REGISTERS + 0x80

; STATE
	THS_PRESENT equ 1
	THS_RUNNABLE equ 2
	THS_ALIVE equ 4
	THS_IOWAIT equ 8
	THS_REGISTRED equ 0x20
	THREAD_READY equ (THS_PRESENT | THS_RUNNABLE | THS_ALIVE | THS_REGISTRED)
	THS_IDLE equ 0x40
	THS_IOPIN equ 0x100
	THS_SLEEP equ 0x200
	THS_MANUAL equ 0x400

extern SystemSpaceBase

KERNEL_CR3 equ 0x9000

SYSTEM_SPACE_CPUMGMT equ 0x200000

APIC_INITIAL_COUNT equ 0x380
APIC_CURRENT_COUNT equ 0x390

CPU_MGMT_SHIFT equ 15


extern TimerClocksPerSecond
extern ApicTimerClockQuantum
extern HpetMainCounterAddress


; HPET Definitions
extern HpetNumClocks ; QWORD

section .text

global SchedulerEntrySSE
global SchedulerEntryAVX
global SchedulerEntryAVX512

; TODO For User mode : DS set to 0x10 at entry, Save & Restore CR3

; SSE Schedule Task Version (SSE3 supported by all X64 Processors)
SchedulerEntrySSE:
	cli
	push rdx
	push rcx
	push rbx
	push rax

	mov rax, [rel SystemSpaceBase]
	mov ebx, [rax + APIC_ID]
	shr ebx, 24
	shl rbx, CPU_MGMT_SHIFT
	lea rax, [rbx + SYSTEM_SPACE_CPUMGMT + rax]

	cmp dword [rax], 0
	je .Exit
	mov rbx, [rax + CPM_CURRENT_THREAD]
	
	; SAVE Schedule Registers
	jmp .SSESaveRegisters
.R0:
	jmp .SSEFindNextTask ; RBX = New Task
.R1:

.Exit:
	mov r8, 0xFAFA
	hlt
	jmp $

.SSEFindNextTask:
	; mov rcx, []
	jmp .R1
.SSESaveRegisters:
	; FXSAVE
	mov rcx, [rbx + _XSAVE]
	fxsave [rcx]
	; PUSHED Registers
	movdqu xmm0, [rsp]
	movdqu [rbx + __MM16_RAXRBX], xmm0
	movdqu xmm0, [rsp + 0x10]
	movdqu [rbx + __MM16_RCXRDX], xmm0
	; SAVE Interrupt Registers
	movdqu xmm0, [rsp + 0x20]
	movdqu [rbx + __MM16_RIPCS], xmm0
	movdqu xmm0, [rsp + 0x30]
	movdqu [rbx + __MM16_RFLAGSRSP], xmm0
	
	; Segment Registers
	mov [rbx + _DS], ds
	mov cx, [rsp + 0x40]
	mov [rbx + _SS], ax
	mov [rbx + _GS], gs
	mov [rbx + _FS], fs
	mov [rbx + _ES], es

	; Remaining Registers
	
	mov [rbx + _RDI], rdi
	mov [rbx + _RSI], rsi
	mov [rbx + _R8], r8
	mov [rbx + _R9], r9
	mov [rbx + _R10], r10
	mov [rbx + _R11], r11
	mov [rbx + _R12], r12
	mov [rbx + _R13], r13
	mov [rbx + _R14], r14
	mov [rbx + _R15], r15
	mov [rbx + _RBP], rbp
	jmp .R0

; TODO : VPINSRQ To mov registers across YMM (Result : 4/3 times faster register switching)
SchedulerEntryAVX:
	
SchedulerEntryAVX512:



section .data

align 0x1000

	PreemptionStatePtr:

	dq PRIORITY_CLASS_INDEX_REALTIME 	; 0
	dq PRIORITY_CLASS_INDEX_HIGH		; 1
	dq PRIORITY_CLASS_INDEX_ABOVE_NORMAL; 2
	dq PRIORITY_CLASS_INDEX_NORMAL		; 3
	dq PRIORITY_CLASS_INDEX_BELOW_NORMAL; 4
	dq PRIORITY_CLASS_INDEX_LOW			; 5
	dq PRIORITY_CLASS_INDEX_IDLE		; 6
	times 100 dq 0xFFFFFFFFFFFFFFFF
	
