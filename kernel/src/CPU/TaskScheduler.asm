
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
	CPM_CPUID equ 0x04
	CPM_THREAD equ 0x08
	CPM_PREEMPTION_STATE equ 0x10
	CPM_SCHEDULER_CPU_TIME equ 0x18
	CPM_ESTIMATED_CPU_TIME equ 0x20
	CPM_THREAD_INDEX equ 0x28
	CPM_CURRENT_LIST equ 0x60
	CPM_IDLE_THREAD equ 0x98
	CPM_TSD equ 0xA0
	CPM_CPUBUFFER equ 0xE8
	CPM_CPUBUFFERSZ equ 0xF0
	CPM_COMMAND equ 0xF8
	CPM_COMMAND_CONTROL equ 0xFC
	CPM_NEXT_THREAD		equ 0x100
	CPM_SYSTEM_INTERRUPTS_THREAD equ 0x108
	CPM_IRQ_CONTROL_TABLE equ 0x110

; TASK SCHEDULER DATA TABLE
	PROCESSING_REGISTER0 equ CPM_TSD
	PROCESSING_REGISTER1 equ CPM_TSD + 8
	__MM16_PROCESSING_REGISTERS_0_1 equ CPM_TSD
	TSD_RIP equ CPM_TSD + 0x10
	TSD_CS equ CPM_TSD + 0x18
	__MM16_TSD_RIPCS equ TSD_RIP

	TSD_SS equ CPM_TSD + 0x20
	TSD_RFLAGS equ CPM_TSD + 0x28
	__MM16_TSD_SSRFLAGS equ TSD_SS
	TSD_RSP equ CPM_TSD + 0x30
	__MM16_TSD_RFLAGSRSP equ TSD_RFLAGS
	TSD_CR3 equ CPM_TSD + 0x38
	TSD_REMAINING_CPU_TIME equ CPM_TSD + 0x40

; THREAD_PRIORITY_PTR_LIST

	TPPL_INDEX equ 0
	TPPL_INDEX_MIN equ 4
	TPPL_INDEX_MAX equ 8
	TPPL_COUNT equ 12
	TPPL_PRIORITY_SPECIFIER equ 16
	TPPL_THREADS equ 20
	TPPL_NEXT equ 820
	;THPTRLISTSZ equ 800

; PROCESS THREAD TABLE
	TH_STATE equ 0
	TH_THREAD_ID equ 0x08
	TH_PROCESS equ 0x10
	TH_PROCESSORID equ 0x18
	TH_THREAD_PRIORITY equ 0x1C
	TH_PREEMPTION_PRIORITY equ 0x20
	TH_TIME_SLICE equ 0x24
	TH_PRIORITY_CLASS_INDEX equ 0x28
	TH_PRIORITY_CLASS_LIST equ 0x2C
	TH_SLEEP_DELAY equ 0x34
	TH_INSTRUCTION_PTR equ 0x3C
	TH_EXIT_CODE equ 0x44
	TH_REGISTERS equ 0x4C

	TH_REGISTER_SIZE equ 0xC0
	TH_STACK equ 0x10C
	TH_RUN_AFTER equ 0x114
	TH_CPU_TIME equ 0x118
	TH_SCHEDULER_CPU_TIME equ 0x120
	TH_VIRTUAL_STACK equ 0x128
	TH_CLIENT equ 0x130
	TH_UNIQUE_CPU equ 0x138
	TH_CONTROL_BIT equ 0x140
	TH_THREAD_PRIORITY_INDEX equ 0x148
	TH_REMOVE_IOWAIT_AFTER   equ 0x150
	TH_ATTEMPT_REMOVE_IOWAIT equ 0x154
	TH_REMAINING_CLOCKS		equ  0x158
	TH_SLEEPUNTIL equ 0x15C


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
section .text

; ScheduleTask:
; 	cli
; 	push rax
; 	mov rax, [rel SystemSpaceBase]
; 	mov ebx, [rax + APIC_ID]
; 	shr ebx, 24

ScheduleTask:
	cli
	
	push rbx
	push rax
	mov rax, [rel SystemSpaceBase] ; + 0 = SYSTEM_SPACE_LAPIC


	mov ebx, [rax + APIC_ID]
	shr rbx, 24

	; set elapsed time to quantum (STAGE 1)
	push rcx
	mov rcx, TimerIncrementerCpuId
	mov ecx, [rcx]
	cmp ecx, ebx
	jne .SkipTimeCalibrateStage1

	push rbx
	push rdx
	push r8


	mov rdx, TimerClocksPerSecond
	lea r8, [rax + SYSTEM_SPACE_CPUMGMT + rbx]
	mov r8, [r8 + CPM_ESTIMATED_CPU_TIME]

	mov rcx, ApicTimerClockCounter

	mov rbx, ApicTimerBaseQuantum

	test r8, r8
	jz .SkipCalibratePrecision


	mov rax, [rdx]
	xor rdx, rdx ; Reset rdx, to divide rdx:rax by r8
	div r8

	mov [rbx], rax

	.SkipCalibratePrecision:

	mov rbx, [rbx] ; JUST lower 32 Bits are used, but for optimization
				   ; The variable is 64 Bit wide to add directly to TimerClocks
				   ; Without conversions
	add [rcx], rbx

	mov rax, [rel SystemSpaceBase]
	mov dword [rax + APIC_INITIAL_COUNT], 0xffffffff
	pop r8
	pop rdx
	pop rbx
	.SkipTimeCalibrateStage1:
	pop rcx

	shl rbx, CPU_MGMT_SHIFT ; shr by 24 then shl by 12 (multiply by 0x1000)


	lea rax, [rax + SYSTEM_SPACE_CPUMGMT + rbx]

	; Save SSE & Extended Registers
	mov rbx, [rax + CPM_THREAD]

	mov rbx, [rbx + _XSAVE]
	fxsave [rbx]

	mov rbx, [rax + CPM_THREAD]
	movdqu xmm0, [rsp]
	movdqu [rbx + __MM16_RAXRBX], xmm0


	add rsp, 0x10	
	; Save RCX & RDX

	movq xmm0, rcx
	movq xmm1, rdx
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_RCXRDX], xmm0


	mov rcx, Pmgrt
	cmp dword [rcx], 0 ; SCHEDULER Enable

	je .Exit

	cmp dword [rbx + TH_REMAINING_CLOCKS], 0
	jne .CpuTimeRemains
	mov rdx, cr3
	mov rcx, rdx
	shr rdx, 12
	cmp rdx, 9 ; 0x9000 (Address of Kernel Page Table)
	je .Skip0
	mov cr3, rdx
	.Skip0:

	mov [rbx + _CR3], rcx

	movdqu xmm0, [rsp]
	movdqu [rbx + __MM16_RIPCS], xmm0

	movdqu xmm0, [rsp + 0x10]
	movdqu [rbx + __MM16_RFLAGSRSP], xmm0
	
	add rsp, 0x20


	pop qword [rbx + _SS]
	mov cx, ds

	mov [rbx + _DS], cx
	
	


	call SaveThreadState
	;call SearchNewThread
	
	mov rbx, [rax + CPM_NEXT_THREAD]
	test rbx, rbx
	jz .sth1
	mov qword [rax + CPM_NEXT_THREAD], 0
	call SearchNewThread.LoadThread
	jmp .sth2
	.sth1:
	call SearchNewThread
	.sth2:
	;call SearchNewThread

	; Setup Stack Frame
	
	sub rsp, 0x28
	movdqu xmm0, [rax + __MM16_TSD_RIPCS]
	movdqu [rsp], xmm0

	movdqu xmm0, [rax + __MM16_TSD_RFLAGSRSP]
	movdqu [rsp + 0x10], xmm0

	mov rbx, [rax + TSD_SS]
	mov [rsp + 0x20], rbx


	mov rbx, [rax + CPM_THREAD]
	mov rcx, Pmgrt
	.Exit:

	cmp dword [rcx + CPU_TIME_CALCULATION], 1
	je .SkipCpuTimeIncrement

	inc qword [rax + CPM_SCHEDULER_CPU_TIME]

	mov rbx, [rax + CPM_THREAD]

	inc qword [rbx + TH_SCHEDULER_CPU_TIME]

	lock inc qword [rcx + SCHEDULER_CPU_TIME]

	test qword [rbx + TH_STATE], THS_IDLE
	jz .NotIdleThread
	inc qword [rcx + IDLE_CPU_TIME]
	.NotIdleThread:

	.SkipCpuTimeIncrement:

	mov rcx, [rbx + _CR3]

	mov rbx, KeGlobalCR3
	mov rbx, [rbx]

	cmp rcx, rbx
	je .SkipTlbFlush
	mov cr3, rcx
	.SkipTlbFlush:

	mov rbx, ApicTimerBaseQuantum
	mov rbx, [rbx] ; Get dword value
	
	mov rcx, [rel SystemSpaceBase]
	bt rbx, 63
	jnc .ResetOneShotMode ; TSC_DEADLINE Is not supported
	; setup TSC_DEADLINE_MSR (0x6E0)
	push rax
	push rdx
	push rcx
	mov ecx, 0x6E0
	mov eax, ebx ; get lower bits of ApicTimerBaseQuantum
	xor edx, edx
	wrmsr ; Write TSC_DEADLINE_MSR
	pop rcx
	pop rdx
	pop rax
	jmp .Finish
	.ResetOneShotMode:

	; TIME CALIBRATE STAGE2 (CALCULATE USED TIME BY INTERRUPT HANDLER)

	push rcx
	push rbx

	push rdx
	mov rbx, TimerIncrementerCpuId
	mov edx, [rax + CPM_CPUID]
	cmp edx, [rbx]
	jne .SkipTimeCalibrateStage2

	; mov rbx, ApicTimerClockCounter
	; mov rdx, [rel SystemSpaceBase]
	; mov rcx, 0xffffffff
	; sub ecx, [rdx + APIC_CURRENT_COUNT]
	; ; shl rcx, 5; MAGIC CALIBRATER VALUE
	;add [rbx], rcx


	.SkipTimeCalibrateStage2:
	pop rdx

	pop rbx
	pop rcx


	mov dword [rcx + APIC_INITIAL_COUNT], ebx

	.Finish:
	mov rbx, [rax + CPM_THREAD]
	push rcx

	; FXSTOR & Restore RDX & RCX
	mov rdx, [rbx + _RDX]
	mov rcx, [rbx + _XSAVE]
	fxrstor [rcx]



	pop rcx



	mov rax, [rbx + _RAX]
	mov dword [rcx + 0xB0], 0 ; APIC_EOI
	mov rcx, [rbx + _RCX]
	mov rbx, [rbx + _RBX]

	
	iretq

.test:
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	mov rdx, 0xAAAAAAAAAAAAA
	cli
	hlt
.CpuTimeRemains:
	dec dword [rbx + TH_REMAINING_CLOCKS]
	jmp .Exit

__InterruptCheckHalt:
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	mov rdx, 0xffffffffffffffff
	.halt:
	cli
	hlt
	jmp .halt
SaveThreadState:
	movq xmm0, rdi
	movq xmm1, rsi
	movlhps xmm0, xmm1
	
	movdqu [rbx + __MM16_RDIRSI], xmm0

	movq xmm0, r8
	movq xmm1, r9
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_R8R9], xmm0

	movq xmm0, r10
	movq xmm1, r11
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_R10R11], xmm0
	
	movq xmm0, r12
	movq xmm1, r13
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_R12R13], xmm0

	movq xmm0, r14
	movq xmm1, r15
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_R14R15], xmm0

	mov [rbx + _RBP], rbp

	mov cx, fs
	shl ecx, 16
	mov cx, gs

	mov [rbx + _GS], ecx ; Set gs, fs
	mov cx, es
	mov [rbx + _ES], cx

	
	; Set ready flag for the thread to be able to be executed another time


	;or qword [rbx + TH_STATE], THS_RUNNABLE
	ret

SearchNewThread:
	mov rcx, [rax + CPM_PREEMPTION_STATE]
	cmp rcx, PREEMPTION_STATE_MAX ; PREEMPTION STATE COUNT * 8
	je .ResetPreemptionState

	.continue0:
	mov rbx, PreemptionStatePtr
	add rcx, rbx
	mov rcx, [rcx] ; get priority class offset

	add qword [rax + CPM_PREEMPTION_STATE], 8
	mov rbx, Pmgrt
	mov rdx, [rbx + THREAD_LIST_PTR_COUNT + rcx]
	test rdx, rdx
	jz SearchNewThread ; After incrementing, retry thread search

	mov r14, [rax + CPM_THREAD]
	mov r14, [r14 + TH_PROCESS]

	mov r11d, [rax + CPM_CPUID]
	.SearchList:
		mov rbx, [rax + CPM_CURRENT_LIST + rcx]
		mov edi, [rax + CPM_THREAD_INDEX + rcx] ; Get thread index
		mov r15, rbx
		add r15, TPPL_THREADS ; get to thread list
		; get to index
		
		shl edi, 3

		add r15, rdi

		shr edi, 3

		mov r10d, [rbx + TPPL_INDEX_MAX]
		.SearchLoop:
			cmp edi, r10d
			jnl .GotoNext

			cmp qword [r15], 0
			je .CancelThread

			mov rsi, [r15]
			cmp dword [rsi + TH_UNIQUE_CPU], r11d
			jne .CancelThread

			mov r8, [rsi]
			mov r9, r8
			and r8, THREAD_READY
			cmp r8, THREAD_READY ; THS_IOWAIT Must not be set
			jne .CancelThread

			test r9, THS_MANUAL
			jnz .CancelThread

			test r9, THS_SLEEP
			jnz .AttemptRemoveSleep

			.continue1:

			test r9, THS_IOWAIT
			jnz .AttemptRemoveIoWait

			jmp .DispatchThread


			.CancelThread:
			add r15, 8
			inc edi

			jmp .SearchLoop
		.AttemptRemoveSleep:
			; Compare if (ApicTimerClockCounter >= SleepUntil)
			mov r13, ApicTimerClockCounter
			mov r13, [r13]
			cmp r13, [rsi + TH_SLEEPUNTIL]
			jna .CancelThread ; cmp < for UNSIGNED Operation
			and qword [rsi], ~(THS_SLEEP)
			jmp .continue1
		.AttemptRemoveIoWait:
			test r9, THS_IOPIN
			jz .CancelThread
			; Remove IOPIN & IOWAIT And run the thread
			and r9, ~(THS_IOWAIT | THS_IOPIN)
			mov [rsi], r9
			jmp .DispatchThread ; Try to dispath the thread as fast as its possible
		.GotoNext:
			mov qword [rax + CPM_THREAD_INDEX + rcx], 0
			mov rbx, [rbx + TPPL_NEXT]
			cmp rbx, 0
			je .GotoInitial

			mov [rax + CPM_CURRENT_LIST + rcx], rbx

			jmp SearchNewThread
		.GotoInitial:
			mov rbx, Pmgrt
			add rbx, LP_INITIAL_PRIORITY_CLASSES
			add rbx, rcx
			mov rbx, [rbx]
			mov [CPM_CURRENT_LIST + rax + rcx], rbx

			jmp SearchNewThread
		.ResetPreemptionState:
			xor rcx, rcx
			mov [rax + CPM_PREEMPTION_STATE], rcx
			jmp .continue0
		.DispatchHalt:
			cli
			hlt
			jmp .DispatchHalt
	.DispatchThread:
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		; TODO When Multiprocessor is supported

		
		; Take Control of the Thread
		
		; Check if another cpu already taked control
		mov r12d, [rsi + TH_RUN_AFTER]
		test r12d, r12d
		jz .ThreadIsntReady
		
		; mov [rsi + TH_PROCESSORID], r11d

		.NoMultiprocessorSpinLock:

		
		.ContinueDispatch:
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


		; After the thread control has been taken
		; We will load its registers
		inc edi


		mov [rax + CPM_THREAD_INDEX + rcx], edi
		;and qword [rsi], ~(THS_RUNNABLE) ; Remove Ready Flag


		mov rbx, rsi

		.LoadThread:


		test qword [rbx + TH_STATE], THS_IDLE
		jz .SkipIdlePreemptionSet
		mov rcx, GlobalThreadPreemptionPriorities
		mov rcx, [rcx + 56] ; Priority index 7 * 8
		mov dword [rbx + TH_RUN_AFTER], ecx
		jmp .SkipPreemptionSet

		.SkipIdlePreemptionSet:

		mov rcx, [rbx + TH_THREAD_PRIORITY_INDEX]
		mov rdi, GlobalThreadPreemptionPriorities
		shl rcx, 3 ; Multiply by 8
		mov rcx, [rdi + rcx]
		mov [rbx + TH_RUN_AFTER], ecx ; Get Thread Priority
		
		.SkipPreemptionSet:


		mov rcx, [rbx + _RIP]
		mov [rax + TSD_RIP], rcx

		mov rbp, [rbx + _RBP]
		mov cx, [rbx + _CS]
		mov [rax + TSD_CS], cx
		

		movdqu xmm0, [rbx + __MM16_RFLAGSRSP]
		movdqu [rax + __MM16_TSD_RFLAGSRSP], xmm0

		;;mov rcx, [rbx + _RFLAGS]
		;;mov [rax + TSD_RFLAGS], rcx
		;;mov rcx, [rbx + _RSP]
		;;mov [rax + TSD_RSP], rcx
		
		

		; Set Time Slice
		mov ecx, [rbx + TH_TIME_SLICE]
		mov [rbx + TH_REMAINING_CLOCKS], ecx

		; Save registers rax, rbx
		movdqu xmm0, [rbx + __MM16_RAXRBX]
		movdqu [rax + __MM16_PROCESSING_REGISTERS_0_1], xmm0
		;;mov rcx, [rbx + _RAX]
		;;mov [rax + PROCESSING_REGISTER0], rcx
		;;mov rcx, [rbx + _RBX]
		;;mov [rax + PROCESSING_REGISTER1], rcx

		
		; Restore the rest of the registers

		mov rbp, [rbx + _RBP]
		;mov rdx, [rbx + _RDX]

		movdqu xmm0, [rbx + __MM16_RDIRSI]
		movhlps xmm1, xmm0
		movq rdi, xmm0
		movq rsi, xmm1

		;;mov rdi, [rbx + _RDI]
		;;mov rsi, [rbx + _RSI]


		movdqu xmm0, [rbx + __MM16_R8R9]
		movhlps xmm1, xmm0
		movq r8, xmm0
		movq r9, xmm1

		movdqu xmm0, [rbx + __MM16_R10R11]
		movhlps xmm1, xmm0
		movq r10, xmm0
		movq r11, xmm1

		movdqu xmm0, [rbx + __MM16_R12R13]
		movhlps xmm1, xmm0
		movq r12, xmm0
		movq r13, xmm1

		movdqu xmm0, [rbx + __MM16_R14R15]
		movhlps xmm1, xmm0
		movq r14, xmm0
		movq r15, xmm1

		mov [rax + CPM_THREAD], rbx

		movdqu xmm0, [rbx + __MM16_SEGSES]

		movq rcx, xmm0 ; Get lower segments
		mov ds, cx
		shr rcx, 16
		mov [rax + TSD_SS], cx
		shr rcx, 16
		mov gs, cx
		shr rcx, 16
		mov fs, cx
		movhlps xmm0, xmm0
		movq rcx, xmm0
		mov es, cx


		mov rcx, [rbx + _CR3]
		mov [rax + TSD_CR3], rcx

		ret
	.ThreadIsntReady:
		dec r12d
		test r12d, r12d
		jz .notreadyyet
		mov [rax + CPM_NEXT_THREAD], rsi
		.notreadyyet:
		mov [rsi + TH_RUN_AFTER], r12d
		;or qword [rsi], THS_RUNNABLE
		jmp .CancelThread
	


SkipTaskSchedule:
	cli

	push rbx
	push rax
	mov rax, [rel SystemSpaceBase] ; + 0 = SYSTEM_SPACE_LAPIC


	mov ebx, [rax + APIC_ID]
	shr rbx, 24
	shl rbx, CPU_MGMT_SHIFT ; shr by 24 then shl by 12 (multiply by 0x1000)

	lea rax, [rax + SYSTEM_SPACE_CPUMGMT + rbx]

	; Save SSE & Extended Registers
	mov rbx, [rax + CPM_THREAD]

	mov rbx, [rbx + _XSAVE]
	fxsave [rbx]

	mov rbx, [rax + CPM_THREAD]
	movdqu xmm0, [rsp]
	movdqu [rbx + __MM16_RAXRBX], xmm0


	add rsp, 0x10	
	; Save RCX & RDX

	movq xmm0, rcx
	movq xmm1, rdx
	movlhps xmm0, xmm1
	movdqu [rbx + __MM16_RCXRDX], xmm0


	mov rcx, Pmgrt
	cmp dword [rcx], 0 ; SCHEDULER Enable

	je .Exit
	; Do not check for remaining clocks
	;;cmp dword [rbx + TH_REMAINING_CLOCKS], 0
	;;jne .CpuTimeRemains
	mov rdx, cr3
	mov rcx, rdx
	shr rdx, 12
	cmp rdx, 9 ; 0x9000 (Address of Kernel Page Table)
	je .Skip0
	mov cr3, rdx
	.Skip0:

	mov [rbx + _CR3], rcx

	movdqu xmm0, [rsp]
	movdqu [rbx + __MM16_RIPCS], xmm0

	movdqu xmm0, [rsp + 0x10]
	movdqu [rbx + __MM16_RFLAGSRSP], xmm0
	
	add rsp, 0x20


	pop qword [rbx + _SS]
	mov cx, ds

	mov [rbx + _DS], cx
	
	


	call SaveThreadState
	;call SearchNewThread
	
	mov rbx, [rax + CPM_NEXT_THREAD]
	test rbx, rbx
	jz .sth1
	mov qword [rax + CPM_NEXT_THREAD], 0
	call SearchNewThread.LoadThread
	jmp .sth2
	.sth1:
	call SearchNewThread
	.sth2:

	; Setup Stack Frame
	
	sub rsp, 0x28
	movdqu xmm0, [rax + __MM16_TSD_RIPCS]
	movdqu [rsp], xmm0

	movdqu xmm0, [rax + __MM16_TSD_RFLAGSRSP]
	movdqu [rsp + 0x10], xmm0

	mov rbx, [rax + TSD_SS]
	mov [rsp + 0x20], rbx


	mov rbx, [rax + CPM_THREAD]
	mov rcx, Pmgrt
	.Exit:

	cmp dword [rcx + CPU_TIME_CALCULATION], 1
	je .SkipCpuTimeIncrement

	;inc qword [rax + CPM_SCHEDULER_CPU_TIME]

	mov rbx, [rax + CPM_THREAD]

	;inc qword [rbx + TH_SCHEDULER_CPU_TIME]

	;inc qword [rcx + SCHEDULER_CPU_TIME]

	test qword [rbx + TH_STATE], THS_IDLE
	jz .NotIdleThread
	inc qword [rcx + IDLE_CPU_TIME]
	.NotIdleThread:

	.SkipCpuTimeIncrement:

	mov rcx, [rbx + _CR3]

	mov rbx, KeGlobalCR3
	mov rbx, [rbx]

	cmp rcx, rbx
	je .SkipTlbFlush
	mov cr3, rcx
	.SkipTlbFlush:

	mov rbx, ApicTimerClockQuantum
	mov rbx, [rbx] ; Get dword value
	
	mov rcx, [rel SystemSpaceBase]
	bt rbx, 63
	jnc .ResetOneShotMode ; TSC_DEADLINE Is not supported
	; setup TSC_DEADLINE_MSR (0x6E0)
	push rax
	push rdx
	push rcx
	mov ecx, 0x6E0
	mov eax, ebx ; get lower bits of ApicTimerBaseQuantum
	xor edx, edx
	wrmsr ; Write TSC_DEADLINE_MSR
	pop rcx
	pop rdx
	pop rax
	jmp .Finish
	.ResetOneShotMode:
	mov dword [rcx + 0x380], ebx

	.Finish:
	mov rbx, [rax + CPM_THREAD]
	push rcx

	; FXSTOR & Restore RDX & RCX
	mov rdx, [rbx + _RDX]
	mov rcx, [rbx + _XSAVE]
	fxrstor [rcx]

	pop rcx



	mov rax, [rbx + _RAX]
	mov dword [rcx + 0xB0], 0
	mov rcx, [rbx + _RCX]
	mov rbx, [rbx + _RBX]

	
	iretq


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
	
