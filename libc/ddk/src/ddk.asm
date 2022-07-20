global __cli
export __cli


global __sti
export __sti

global __hlt
export __hlt

global __setRFLAGS
export __setRFLAGS

global __getRFLAGS
export __getRFLAGS

global OutPortB
export OutPortB

global InPortB
export InPortB

global OutPortW
export OutPortW

global InPortW
export InPortW

global OutPort
export OutPort

global InPort
export InPort

global __pause
export __pause

global __setCR3
export __setCR3

global __lidt
export __lidt

global __lgdt
export __lgdt

global GetCurrentProcessorId
export GetCurrentProcessorId



global __getCR2
export __getCR2

global __getCR3
export __getCR3


global __ReadMsr
export __ReadMsr

global __WriteMsr
export __WriteMsr

global __SpinLockSyncBitTestAndSet
export __SpinLockSyncBitTestAndSet

global __BitRelease
export __BitRelease

global __SyncBitTestAndSet
export __SyncBitTestAndSet

global __SyncIncrement64
export __SyncIncrement64

global __SyncIncrement32
export __SyncIncrement32

global __SyncIncrement16
export __SyncIncrement16

global __SyncIncrement8
export __SyncIncrement8

global __SyncDecrement64
export __SyncDecrement64

global __SyncDecrement32
export __SyncDecrement32

global __SyncDecrement16
export __SyncDecrement16

global __SyncDecrement8
export __SyncDecrement8

global __repstos
export __repstos

global __repstos16
export __repstos16

global __repstos32
export __repstos32

global __repstos64
export __repstos64

global __SyncOr
export __SyncOr

global __SyncAnd
export __SyncAnd

[BITS 64]



section .text

__SyncAnd:
	mov r8, rdx

	mov rdx, rcx
	mov rcx, r8
	mov r8, 1
	shl r8, cl
	lock and [rdx], r8
	ret

__SyncOr: ; rcx = Address, edx = BitOffset
	mov r8, rdx

	mov rdx, rcx
	mov rcx, r8
	mov r8, 1
	shl r8, cl
	lock or [rdx], r8
	ret

__ldmxcsr:
	push rcx
	ldmxcsr [rsp]
	pop rcx
	ret

__stmxcsr:
	push 0
	stmxcsr [rsp]
	pop rax
	ret
__wbinvd:
	wbinvd
	ret

__rdtsc:
	xor rax, rax
	rdtsc
	shl rdx, 32
	or rax, rdx
	ret

__InvalidatePage:
	invlpg [rcx]
	ret

__repstos:
	mov rdi, rcx ; Address
	mov al, dl ; Value
	mov rcx, r8 ; count
	rep stosb
	ret

__repstos16:
	mov rdi, rcx ; Address
	mov ax, dx ; Value
	mov rcx, r8 ; count
	rep stosw
	ret

__repstos32:
	mov rdi, rcx ; Address
	mov eax, edx ; Value
	mov rcx, r8 ; count
	rep stosd
	ret

__repstos64:
	mov rdi, rcx ; Address
	mov rax, rdx ; Value
	mov rcx, r8 ; count
	rep stosq
	ret

; BOOL __SyncBitTestAndSet(void* Address, UINT16 BitOffset)
__SyncIncrement64:
	lock inc qword [rcx]
	ret
__SyncIncrement32:
	lock inc dword [rcx]
	ret
__SyncIncrement16:
	lock inc word [rcx]
	ret
__SyncIncrement8:
	lock inc byte [rcx]
	ret

__SyncDecrement64:
	lock dec qword [rcx]
	ret

__SyncDecrement32:
	lock dec dword [rcx]
	ret

__SyncDecrement16:
	lock dec word [rcx]
	ret

__SyncDecrement8:
	lock dec byte [rcx]
	ret


__SyncBitTestAndSet:
	and rdx, 0b111 ; Limit is 7
	lock bts qword [rcx], rdx
	pushf
	pop rax
	and rax, 1 ; Gets Carry Flag
	not eax ; Flips it if 1 then fail (returns 0)
	ret
	

__SpinLockSyncBitTestAndSet:
	and rdx, 0b111 ; Limit is 7
	
	push rcx
	mov r8, 1
	mov rcx, rdx
	shl r8, cl
	pop rcx
	._Lock:
	test [rcx], r8
	jnz ._Lock
	lock bts qword [rcx], rdx
	jc ._Lock ; If set by another processor or normally
	ret

; __BitRelease(void* Address, UINT16 BitOffset)

__BitRelease:
	and rdx, 0b111 ; Limit is 7
	btr qword [rcx], rdx
	ret

; ReadMsr(ecx MsrNumber, eax*, edx*)
__ReadMsr:
	push rdx
	push r8
	; ecx is already set
	rdmsr
	mov rbx, [rsp]
	cmp rbx, 0
	je .skip0
	mov [rbx], edx
	.skip0:
	add rsp, 8
	mov rbx, [rsp]
	cmp rbx, 0
	je .skip1
	mov [rbx], eax
	.skip1:
	add rsp, 8
	ret
__WriteMsr:
	; ecx is already set
	mov eax, edx
	mov edx, r8d
	wrmsr
	ret
__setCR3:
	mov cr3, rcx
	ret
__getCR3:
	mov rax, cr3
	ret
__getCR2:
	xor rax, rax
	mov rax, cr2
	ret

GetCurrentProcessorId:
	push rbx
	push rcx
	push rdx
	mov eax, 1
	cpuid
	shr ebx, 24
	mov eax, ebx
	pop rdx
	pop rcx
	pop rbx
	ret
__pause:
	pause
	ret
__lidt:
	lidt [rcx]
	ret
__lgdt:
	lgdt [rcx]
	ret
__cli:
	cli
	ret
__sti:
	sti
	ret
__hlt:
	hlt
	ret
__setRFLAGS:
	push rcx
	popfq
	ret
__getRFLAGS:
	pushfq
	pop rax
	ret
OutPortB:
	mov ax, dx
	mov dx, cx
	out dx, al
	ret
InPortB:
	mov dx, cx
	in al, dx
	ret
OutPortW:
	mov ax, dx
	mov dx, cx
	out dx, ax
	ret
InPortW:
	mov dx, cx
	in ax, dx
	ret
OutPort:
	mov eax, edx
	mov dx, cx
	out dx, eax
	ret
InPort:
	mov dx, cx
	in eax, dx
	ret
