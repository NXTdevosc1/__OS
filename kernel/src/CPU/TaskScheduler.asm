
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
	push rax
	mov rbx, rax
	xor rdx, rdx
	mov rdi, HpetNumClocks
	mov rcx, [rdi]
	mov rdi, HpetFrequency
	mov rax, [rdi]
	mul rcx
	mov rdi, HpetMainCounterAddress
	add rax, [rdi]
	pop rax
	jmp .SSEFindNextTask ; RBX = New Task
.R1:
	movdqu xmm0, [rax + CPM_TOTAL_CLOCKS]
	mov rcx, 1
	movq xmm1, rcx
	addps xmm0, xmm1
	movdqu [rax + CPM_TOTAL_CLOCKS], xmm0
.Exit:
	
	mov r8, 0xFAFA
	hlt
	jmp $

.SSEFindNextTask: ; Rbx = (IN : Current) (OUT : Selected) Thread
	lea rcx, [rax + CPM_NUM_READY_THREADS] ; XMM0
	lea rdx, [rax + CPM_THREAD_QUEUES] ; XMM1
	lea r8, [rax + CPM_READY_ON_CLOCK] ; XMM2
	lea r9, [rax + CPM_HIGHEST_PRIORITY_THREAD] ; XMM3
	mov r11d, [rbx + TH_THREAD_PRIORITY]

	movdqu xmm4, [rax + CPM_TOTAL_CLOCKS]

xor r15, r15 ; Highest priority index
mov r14, [rax + CPM_HIGH_PRECISION_TIME] ; Used to check Sleep State
.A0L0:

	; Load XMM0-XMM3 With values
	movdqu xmm0, [rcx] ; Num Ready Threads
	movdqu xmm1, [rdx] ; Thread Queues
	movdqu xmm3, [r9] ; Highest Priority Thread

	; Test Ready Threads
	movq rdi, xmm0
	test rdi, rdi
	jnz SSEHighestPriorityThreadScan
	; test Ready On Clock
	movdqu xmm2, [r8]
	
	cmpps xmm2, xmm4, 0
	jae SSEFullScan ; Queue have not ready threads

.A0L0A1:
	; Partial scan on the highest priority ready thread (ReadyThreads > 0)
.A0L0E0:
	sub r8, 0x10 ; Ready on clock
	psrldq xmm0, 8 ; Shift right by 8 bytes
	movq rdi, xmm0
	test rdi, rdi
	jz .A0L0E1

.A0L0E1:
	jmp .R1




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
	mov rax, 0xFFAF
	hlt
	jmp $

SSEFullScan:
	; Full scan on ready threads (ReadyOnClock)
	movq r10, xmm1
	xor rsi, rsi ; Counter to next queue

.loop:
	test rsi, 0x200
	jz .__E0
	mov r10, [r10]
	test r10, r10
	jz .Exit
	xor rsi, rsi
.__E0:
	mov r12, [r10]

	test r12, r12
	jz .__E1
	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jbe .__E1
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
	mov r11, [rbx + TH_THREAD_PRIORITY]
.__E1:
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
.Exit:
	mov rax, 0xFAEFAE
	jmp $

; TODO : VPINSRQ To mov registers across YMM (Result : 4/3 times faster register switching)
SchedulerEntryAVX:
SchedulerEntryAVX512:
