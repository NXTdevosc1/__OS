
[BITS 64]
%include "src\CPU\schedulerdefs.asm"

section .text

global SchedulerEntrySSE


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



	
	mov rbx, [rax + CPM_CURRENT_THREAD]


	test qword [rbx], THS_IDLE
	jnz .R0 ; Idle threads like System Interrupt & Idle thread do not need register saving
	; SAVE Schedule Registers
	jmp .SSESaveRegisters

.R0:
	

	movdqa xmm4, [rax + CPM_TOTAL_CLOCKS]
	
	mov rcx, rax
	xor rdx, rdx
	
	mov rax, [rel HpetNumClocks]
	mul qword [rel HpetFrequency]
	mov rdx, [rel HpetMainCounterAddress]
	add rax, [rdx]
	mov rdx, rax
	mov rax, rcx
	mov [rax + CPM_HIGH_PRECISION_TIME], rdx
	movq xmm13, rdx

	
	cmp qword [rax + CPM_FORCE_NEXT_THREAD], 0
	je .A0

	mov r10, [rbx + TH_PROCESS]
	mov r10d, [r10 + PROCESS_PRIORITY_CLASS]
	shl r10, 3
	dec qword [rax + CPM_NUM_READY_THREADS + r10]
	mov ecx, [rbx + TH_SCHEDULING_QUANTUM]
	movq rdx, xmm4
	add rcx, rdx
	mov [rbx + TH_READY_AT], rcx


	cmp qword [rax + CPM_NUM_READY_THREADS + r10], 0
	jne .A1
	shl r10, 1
	mov [rax + CPM_READY_ON_CLOCK + r10], rcx
.A1:


	mov rbx, [rax + CPM_FORCE_NEXT_THREAD]
	mov qword [rax + CPM_FORCE_NEXT_THREAD], 0
	
	jmp .R1
.A0:
	; Calculate high precision time (As GetHighPrecisionTimeSinceBoot())
	
	

	jmp .SSEFindNextTask ; RBX = New Task
.R1:
	

	; XMM4 Already set with total clocks from .SSEFindNextTask
	inc qword [rax + CPM_TOTAL_CLOCKS]

; Load registers
	mov rsi, [rbx + _RSI]
	mov r8, [rbx + _R8]
	mov r9, [rbx + _R9]
	mov r10, [rbx + _R10]
	mov r11, [rbx + _R11]
	mov r12, [rbx + _R12]
	mov r13, [rbx + _R13]
	mov r14, [rbx + _R14]
	mov r15, [rbx + _R15]


	mov rcx, [rbx + _DS]
	mov ds, cx
	shr rcx, 16
	mov rdx, rcx
	and rdx, 0xFFFF
	push rdx ; Stack Segment
	shr rcx, 16
	mov gs, cx
	shr rcx, 16
	mov fs, cx

	sub rsp, 0x20
	movdqu xmm0, [rbx + __MM16_RIPCS]
	movdqu [rsp], xmm0
	movdqu xmm0, [rbx + __MM16_RFLAGSRSP]
	movdqu [rsp + 0x10], xmm0

	; Calculate Interrupt latency (Only for debugging and optimizing purpose & maybe removed later)
	
	
	mov rcx, rax
	xor rdx, rdx
	
	mov rax, [rel HpetNumClocks]
	mul qword [rel HpetFrequency]
	mov rdx, [rel HpetMainCounterAddress]
	add rax, [rdx]
	mov rdx, rax
	mov rax, rcx

	movq rcx, xmm13
	sub rdx, rcx
	mov [rax + CPM_LAST_THREAD_SWITCH_LATENCY], rdx

	mov rcx, [rbx + _XSAVE]
	fxrstor [rcx]

	

	mov rdi, [rbx + _RDI]
	mov rbp, [rbx + _RBP]
	mov rdx, [rbx + _RDX]
	mov rcx, [rbx + _RCX]
	
	mov [rax + CPM_CURRENT_THREAD], rbx


	; Send APIC EOI
	mov rax, [rel SystemSpaceBase]
	mov dword [rax + 0xB0], 0
	
	mov rax, [rbx + _RAX]
	mov rbx, [rbx + _RBX]
	



	iretq




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
	mov cx, fs
	shl rcx, 16
	mov cx, gs
	shl rcx, 16
	mov cx, [rsp + 0x40]
	shl rcx, 16
	mov cx, ds

	mov [rbx + _DS], rcx ; DS, SS, GS, FS
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

.SSEFindNextTask: ; Rbx = (IN : Current) (OUT : Selected) Thread
	
	

	; Get thread process priority class
	cmp dword [rbx + TH_REMAINING_CLOCKS], 0
	jne .J1
	; Set last priority class to -1 to never compare with another threads (if a ready thread is found then it will be run)
	mov rcx, 0xFFFFFFFFFFFFFFFF
	movq mm5, rcx

	mov r10, [rbx + TH_PROCESS]
	mov r10d, [r10 + PROCESS_PRIORITY_CLASS]
	shl r10, 3
	dec qword [rax + CPM_NUM_READY_THREADS + r10]

	mov ecx, [rbx + TH_SCHEDULING_QUANTUM]
	movq rdx, xmm4
	add rcx, rdx
	mov [rbx + TH_READY_AT], rcx

	cmp qword [rax + CPM_NUM_READY_THREADS], 0
	jne .J4
	; Set Ready On Clocks
	shl r10, 1
	mov [rax + CPM_READY_ON_CLOCK + r10], rcx
.J4:
	
	pxor mm4, mm4
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



; MM0 : Total Threads
; MM1 : Remaining Threads in Search Function

mov r15d, [rbx + TH_REMAINING_CLOCKS]


; xor rbx, rbx ; Zero Selected Thread

lea r13, [rax + CPM_TOTAL_THREADS]
movq mm6, r13

xor r15, r15 ; Priority index


xor rbx, rbx
mov r11d, 0xFF ; Set highest priority number
.loop:
	test r15, 8
	jnz .SSEFindNextTaskEXIT

	test r15, 1 ; Don't load values on second read (XMM Already has bitshifted value)
	jnz .SkipXMMLoad

	movq r13, mm5
	cmp r15, r13
	jne .A2
	movq rbx, mm4 ; Compare current thread with other threads to invoke preemption
.A2:

	

	; Load XMM0-XMM2 With values
	movdqa xmm0, [rcx] ; Num Ready Threads
	movdqa xmm1, [rdx] ; Thread Queues

	movq r13, mm6
	movdqa xmm15, [r13] ; Total Threads
	
	
	add rdx, 0x10
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
	movq r13, xmm4
	cmp r13, [r8]
	jb .A0L0E0
	mov r9, [rcx]
	call SSEFullScan
	mov [rcx], r9
	; Partial scan on the highest priority ready thread (ReadyThreads > 0)
.A0L0E0:
	
	test rbx, rbx
	jnz .SSEFindNextTaskEXIT

	add r8, 0x10 ; Ready on clock
	add rcx, 0x8
	
	psrldq xmm0, 8 ; Shift right by 8 bytes
	psrldq xmm1, 8
	psrldq xmm15, 8

	inc r15
	

	jmp .loop


.SSEFindNextTaskEXIT:
	
	test rbx, rbx
	jz .SetIdleThread

	test rax, 0x8000
	jz .b
	mov rcx, 0xCAFE
	jmp $
.b:

	movq rcx, mm4
	cmp rcx, rbx
	jne .J3
	dec dword [rbx + TH_REMAINING_CLOCKS]
	jmp .R1
.J3:
	mov ecx, [rbx + TH_TIME_BURST]
	mov [rbx + TH_REMAINING_CLOCKS], ecx
	mov [rax + CPM_CURRENT_THREAD], rbx

	movq rcx, mm4
	test rcx, rcx
	jz .R1
	; Release preempted thread
	mov r10, [rcx + TH_PROCESS]
	mov r10d, [r10 + PROCESS_PRIORITY_CLASS]
	shl r10, 3
	dec qword [rax + CPM_NUM_READY_THREADS + r10]

	mov edi, [rcx + TH_SCHEDULING_QUANTUM]
	movq rsi, xmm4
	add rdi, rsi
	mov [rcx + TH_READY_AT], rdi

	cmp qword [rax + CPM_NUM_READY_THREADS], 0
	jne .R1
	; Set Ready On Clocks
	shl r10, 1
	mov [rax + CPM_READY_ON_CLOCK + r10], rdi
	jmp .R1
.SetIdleThread:

	mov rbx, [rax + CPM_SYSTEM_IDLE_THREAD]
	mov [rax + CPM_CURRENT_THREAD], rbx
	inc qword [rax + CPM_TOTAL_CLOCKS]
	mov rax, [rel SystemSpaceBase]
	mov dword [rax + 0xB0], 0 ; APIC_EOI
	; Idle cpu time
	sti
	jmp .__Idle
	; mov rax, .__Idle
	; push 0x10
	; push rsp ; Anyway it will not be used
	; push 0x200
	; push 0x08
	; push rax
	; iretq

.__Idle:
	mov rax, 0xCAFE
	hlt
.SchedulerOverflow:
	cli
	mov rax, 0xDBBEEF
	hlt
	jmp .SchedulerOverflow
; ______________________________________________________________________







SSEHighestPriorityThreadScan:
		; Full scan on ready threads (ReadyOnClock)
	movq r10, xmm1
	movq rdi, xmm4
	xor rsi, rsi ; Counter to next queue
	
	movq rdi, mm0 ; total threads
	
.loop:
	test rsi, 0x200
	jz .__E0
	mov r10, [r10]
	test r10, r10
	jz .Exit
	xor rsi, rsi
.__E0:
	test rdi, rdi
	jz .Exit

	movdqu xmm10, [r10]

	movq r12, xmm10

	test r12, r12
	jz .__E1

	dec rdi
	
	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .__E1
	movq r13, xmm4
	cmp r13, [r12 + TH_READY_AT]
	jb .__E1

	mov r13, [r12] ; State
	test r13, THS_RUNNABLE | THS_ALIVE
	jz .__E1
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
	test r11d, r11d
	jz .Exit ; 0 is the highest priority availaible
.__E1:
	psrldq xmm10, 8

.SSESecondRead:
	; No need for rsi check because it's aligned
	test rdi, rdi
	jz .Exit

	movq r12, xmm10

	test r12, r12
	jz .E1
	
	dec rdi

	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .E1
	movq r13, xmm4
	cmp r13, [r12 + TH_READY_AT]
	jb .E1
	mov r13, [r12] ; State
	test r13, THS_RUNNABLE | THS_ALIVE
	jz .__E1
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
	test r11d, r11d
	jz .Exit ; 0 is the highest priority availaible
.E1:
	add rsi, 2
	add r10, 0x10
	jmp .loop

.IOWAIT:
	test r13, THS_IOPIN
	jz .__E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .__E2
.SLEEP:
	movq rdi, xmm13
	cmp rdi, [r12 + TH_SLEEP_UNTIL]
	jb .__E1
	and qword [r12], ~(THS_SLEEP)
	jmp .__E3
.IOWAIT2:
	test r13, THS_IOPIN
	jz .E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .E2
.SLEEP2:
	movq rdi, xmm13
	cmp rdi, [r12 + TH_SLEEP_UNTIL]
	jb .E1
	and qword [r12], ~(THS_SLEEP)
	jmp .E3

.Exit:
	ret




SSEFullScan:
	; Full scan on ready threads (ReadyOnClock)
	movq r10, xmm1
	movq rdi, xmm4
	
	xor rsi, rsi ; Counter to next queue
	
	movq rdi, mm0
.loop:
	test rsi, 0x200
	jz .__E0
	mov r10, [r10]
	test r10, r10
	jz .Exit
	xor rsi, rsi
.__E0:
	test rdi, rdi
	jz .Exit

	movdqu xmm10, [r10]

	movq r12, xmm10

	test r12, r12
	jz .__E1
	
	dec rdi

	movq r13, xmm4
	cmp r13, [r12 + TH_READY_AT]
	jb .__E1

	inc r9 ; NUM_READY_THREADS
	
	cmp [r12 + TH_THREAD_PRIORITY], r11d ; 0 = Highest, 6 = lowest
	jae .__E1
	mov r13, [r12] ; State
	test r13, THS_RUNNABLE | THS_ALIVE
	jz .__E1
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
.SSESecondRead:
	; No need for rsi check because it's aligned
	test rdi, rdi
	jz .Exit

	movq r12, xmm10

	test r12, r12
	jz .E1
	
	dec rdi

	movq r13, xmm4
	cmp r13, [r12 + TH_READY_AT]
	jb .E1

	inc r9 ; NUM_READY_THREADS

	cmp [r12 + TH_THREAD_PRIORITY], r11d
	jae .E1
	mov r13, [r12] ; State
	test r13, THS_RUNNABLE | THS_ALIVE
	jz .E1
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
	add rsi, 2
	add r10, 0x10
	jmp .loop

.IOWAIT:
	test r13, THS_IOPIN
	jz .__E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .__E2
.SLEEP:
	movq rdi, xmm13
	cmp rdi, [r12 + TH_SLEEP_UNTIL]
	jb .__E1
	and qword [r12], ~(THS_SLEEP)
	jmp .__E3
.IOWAIT2:
	test r13, THS_IOPIN
	jz .E1
	and qword [r12], ~(THS_IOWAIT | THS_IOPIN)
	jmp .E2
.SLEEP2:
	movq rdi, xmm13
	cmp rdi, [r12 + TH_SLEEP_UNTIL]
	jb .E1
	and qword [r12], ~(THS_SLEEP)
	jmp .E3

.Exit:
	ret

