
[BITS 64]
%include "src\CPU\schedulerdefs.asm"

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
	; Calculate high precision time (As GetHighPrecisionTimeSinceBoot())
	mov r8, rbx
	mov rbx, rax
	xor rdx, rdx
	mov rdi, HpetNumClocks
	mov rcx, [rdi]
	mov rdi, HpetFrequency
	mov rax, [rdi]
	mul rcx
	mov rdi, HpetMainCounterAddress
	add rax, [rdi]
	mov rax, rbx
	mov rbx, r8

	jmp .SSEFindNextTask ; RBX = New Task
.R1:
	; XMM4 Already set with total clocks from .SSEFindNextTask
	mov rcx, 1
	movq xmm7, rcx
	addps xmm4, xmm7
	movdqa [rax + CPM_TOTAL_CLOCKS], xmm4
.Exit:
	
	mov r8, 0xFAFA
	hlt
	jmp $




; ______________________________________________________________________


.SSEFindNextTask: ; Rbx = (IN : Current) (OUT : Selected) Thread
	
	; Get thread process priority class
	cmp qword [rbx + TH_REMAINING_CLOCKS], 0
	jne .J1
	; Set last priority class to -1 to never compare with another threads (if a ready thread is found then it will be run)
	mov rcx, 0xFFFFFFFFFFFFFFFF
	movq mm5, rcx
	mov ecx, [rbx + TH_SCHEDULING_QUANTUM]
	movdqa xmm0, xmm4 ; Copy Current Clocks
	movq xmm1, rcx
	addps xmm0, xmm1
	movdqu [rbx + TH_READY_AT], xmm0
	mov ecx, [rbx + TH_TIME_BURST]
	mov [rbx + TH_REMAINING_CLOCKS], ecx
	; Set Ready At to Current Time + Thread Quantum
	jmp .J2
.J1:
	; Otherwise the thread can be preempted
	mov rcx, [rbx + TH_PROCESS]
	mov ecx, [rcx + PROCESS_PRIORITY_CLASS]
	movq mm5, rcx
	movq mm4, rbx
.J2:

	lea rcx, [rax + CPM_NUM_READY_THREADS] ; XMM0
	lea rdx, [rax + CPM_THREAD_QUEUES] ; XMM1
	lea r8, [rax + CPM_READY_ON_CLOCK] ; XMM2
	lea r9, [rax + CPM_HIGHEST_PRIORITY_THREAD] ; XMM3

	movdqa xmm4, [rax + CPM_TOTAL_CLOCKS]

mov r14, [rax + CPM_HIGH_PRECISION_TIME] ; Used to check Sleep State

; MM0 : Total Threads
; MM1 : Remaining Threads in Search Function

mov r15, [rbx + TH_REMAINING_CLOCKS]


; xor rbx, rbx ; Zero Selected Thread

lea r13, [rax + CPM_TOTAL_THREADS]
movq mm6, r13

xor r15, r15 ; Priority index

xor rbx, rbx
.loop:
	test r15, 8
	jnz .SSEFindNextTaskEXIT

	test r15, 1 ; Don't load values on second read (XMM Already has bitshifted value)
	jnz .SkipXMMLoad

	; Load XMM0-XMM3 With values
	movdqa xmm0, [rcx] ; Num Ready Threads
	movdqa xmm1, [rdx] ; Thread Queues
	movdqa xmm3, [r9] ; Highest Priority Thread
	movq r13, mm6
	movdqa xmm15, [r13] ; Total Threads
	
	add rcx, 0x10
	add rdx, 0x10
	add r8, 0x10
	add r9, 0x10
	add r13, 0x10
	movq mm6, r13 ; R13 is changeable by functions (MM6 Preserves R13 Value)
.SkipXMMLoad:
	movq r13, mm5
	cmp r15, r13
	jne .J0
	; Set comparision thread
	movq rbx, mm4 ; Compare thread with threads on the same priority class
.J0:


	movq r13, xmm15
	movq mm0, r13

	; Test Ready Threads
	movq rdi, xmm0
	test rdi, rdi
	jz .A0L0A1
	call SSEHighestPriorityThreadScan
	jmp .A0L0E0
.A0L0A1:
	; test Ready On Clock
	; movdqu xmm2, [r8]
	movq rdi, xmm4
	cmp [r8], rdi
	jb .A0L0E0 ; Queue have not ready threads
	call SSEFullScan
	; Partial scan on the highest priority ready thread (ReadyThreads > 0)
.A0L0E0:
	add r8, 0x10 ; Ready on clock
	inc r15


	psrldq xmm0, 8 ; Shift right by 8 bytes
	psrldq xmm1, 8
	psrldq xmm3, 8
	psrldq xmm15, 8
	
	jmp .loop


.SSEFindNextTaskEXIT:
	mov rax, 0xDEADCAFE
	jmp $

; ______________________________________________________________________





.SSESaveRegisters:
	; FXSAVE
	mov rcx, [rbx + _XSAVE]
	fxsave [rcx] ; SSE Likely does not support XSAVE

	
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

SSEHighestPriorityThreadScan:
		; Full scan on ready threads (ReadyOnClock)
	movq r10, xmm1
	movq rdi, xmm2
	xor rsi, rsi ; Counter to next queue
	
	movq r13, mm0
	movq xmm8, r13
	
	mov r13, 1
	movq xmm9, r13 ; Substraction value
.loop:
	test rsi, 0x200
	jz .__E0
	mov r10, [r10]
	test r10, r10
	jz .Exit
	xor rsi, rsi
.__E0:
	movq r13, xmm8
	test r13, r13
	jz .Exit

	movdqu xmm10, [r10]

	movq r12, xmm10

	test r12, r12
	jz .__E1
	psubq xmm8, xmm9
	cmp [r12 + TH_READY_AT], rdi
	jb .__E1
	
	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .__E1
	mov r13, [r12] ; State
	test r13, THS_MANUAL | THS_IDLE
	jnz .__E1
	test r13, THS_SLEEP
	jnz .SLEEP
.__E3:
	test r13, THS_IOWAIT
	jnz .IOWAIT
.__E2:
	
	mov rbx, r12
	mov r11d, [rbx + TH_THREAD_PRIORITY]
	movq r13, xmm3 ; Highest Priority
	cmp r11d, r13d
	je .Exit
.__E1:
	psrldq xmm10, 8
	inc rsi
	add r10, 8
.SSESecondRead:
	; No need for rsi check because it's aligned
	movq r13, xmm8
	test r13, r13
	jz .Exit

	movq r12, xmm10

	test r12, r12
	jz .E1
	psubq xmm8, xmm9
	cmp [r12 + TH_READY_AT], rdi
	jb .E1
	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .E1
	mov r13, [r12] ; State
	test r13, THS_MANUAL | THS_IDLE
	jnz .E1
	test r13, THS_SLEEP
	jnz .SLEEP2
.E3:
	test r13, THS_IOWAIT
	jnz .IOWAIT2
.E2:
	
	mov rbx, r12
	mov r11d, [rbx + TH_THREAD_PRIORITY]
	movq r13, xmm3 ; Highest Priority
	cmp r11d, r13d
	je .Exit
.E1:
	inc rsi
	add r10, 8
	jmp .loop

.IOWAIT:
	test r13, THS_IOPIN
	jz .__E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .__E2
.SLEEP:
	
	cmp r14, [r10 + TH_SLEEP_UNTIL]
	ja .__E3
	and qword [r10], ~(THS_SLEEP)
	jmp .__E1
.IOWAIT2:
	test r13, THS_IOPIN
	jz .E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .E2
.SLEEP2:
	
	cmp r14, [r10 + TH_SLEEP_UNTIL]
	ja .E3
	and qword [r10], ~(THS_SLEEP)
	jmp .E1

.Exit:
	ret



; TODO : 128-BIT Compare XMM2

SSEFullScan:
	; Full scan on ready threads (ReadyOnClock)
	movq r10, xmm1
	movq rdi, xmm2
	xor rsi, rsi ; Counter to next queue
	
	movq r13, mm0
	movq xmm8, r13
	
	mov r13, 1
	movq xmm9, r13 ; Substraction value
.loop:
	test rsi, 0x200
	jz .__E0
	mov r10, [r10]
	test r10, r10
	jz .Exit
	xor rsi, rsi
.__E0:
	movq r13, xmm8
	test r13, r13
	jz .Exit

	movdqu xmm10, [r10]

	movq r12, xmm10

	test r12, r12
	jz .__E1
	psubq xmm8, xmm9
	cmp [r12 + TH_READY_AT], rdi
	jb .__E1
	inc qword [rcx] ; Num Ready Threads
	cmp [r12 + TH_THREAD_PRIORITY], r11d ; 0 = Highest, 6 = lowest
	jae .__E1
	mov r13, [r12] ; State
	test r13, THS_MANUAL | THS_IDLE
	jnz .__E1
	test r13, THS_SLEEP
	jnz .SLEEP
.__E3:
	test r13, THS_IOWAIT
	jnz .IOWAIT
.__E2:
	
	mov rbx, r12
	mov r11d, [rbx + TH_THREAD_PRIORITY]
.__E1:
	psrldq xmm10, 8
	inc rsi
	add r10, 8
.SSESecondRead:
	; No need for rsi check because it's aligned
	movq r13, xmm8
	test r13, r13
	jz .Exit

	movq r12, xmm10

	test r12, r12
	jz .E1
	psubq xmm8, xmm9
	cmp [r12 + TH_READY_AT], rdi
	jb .E1
	inc qword [rcx] ; Num Ready Threads
	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .E1
	mov r13, [r12] ; State
	test r13, THS_MANUAL | THS_IDLE
	jnz .E1
	test r13, THS_SLEEP
	jnz .SLEEP2
.E3:
	test r13, THS_IOWAIT
	jnz .IOWAIT2
.E2:
	
	mov rbx, r12
	mov r11d, [rbx + TH_THREAD_PRIORITY]
.E1:
	inc rsi
	add r10, 8
	jmp .loop

.IOWAIT:
	test r13, THS_IOPIN
	jz .__E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .__E2
.SLEEP:
	
	cmp r14, [r10 + TH_SLEEP_UNTIL]
	ja .__E3
	and qword [r10], ~(THS_SLEEP)
	jmp .__E1
.IOWAIT2:
	test r13, THS_IOPIN
	jz .E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .E2
.SLEEP2:
	
	cmp r14, [r10 + TH_SLEEP_UNTIL]
	ja .E3
	and qword [r10], ~(THS_SLEEP)
	jmp .E1

.Exit:
	ret

; TODO : VPINSRQ To mov registers across YMM (Result : 4/3 times faster register switching)
SchedulerEntryAVX:
SchedulerEntryAVX512:
