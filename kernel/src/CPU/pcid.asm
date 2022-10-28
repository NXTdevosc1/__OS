[BITS 64]


global 	CheckProcessContextIdEnable
global 	ProcessContextIdDisable
global	CheckInvPcidSupport
section .text

CheckProcessContextIdEnable:
	mov rax, 0xca0
	; Check pcid Support in IA32_EFER
	xor rcx, rcx
	mov eax, 1
	cpuid

	test ecx, 1 << 17 ; PCID Support
	jnz .EnablePCID

	; NO PCID
	xor eax, eax
	ret

	.EnablePCID:
	;mov ecx, 0xC0000080 ; IA32_EFER MSR
	;rdmsr
	;mov ecx, 0xC0000080
	;or eax, 1 << 15 ; PCID (Translation Cache) Extension Enable
	;wrmsr

	mov rax, cr4
	or rax, 1 << 17 ; Set PCID Enable Flag
	mov cr4, rax
	mov eax, 1 ; Return TRUE
	ret

CheckInvPcidSupport:
	mov rax, 0xca0
	jmp $
	xor rcx, rcx
	mov eax, 7
	cpuid

	test ebx, 1 << 10 ; INVPCID Support
	jnz .InvPCIDSupported

	xor eax, eax ; return FALSE
	ret

	.InvPCIDSupported:
	mov eax, 1 ; return TRUE
	ret

InvalidateProcessContextId:
	; Invalidation Type 1 (Invalidate all TLB Associated with the PCID)
	and rcx, 0xfff; AND MASK RCX
	push rcx ; PCID | Reserved << 12
	push 0 ; Reserved Linear Address (only used in Type 0)
	mov rdx, 1
	invpcid rdx, [rsp]
	pop rcx
	pop rcx
	ret


ProcessContextIdDisable:
	mov rax, cr4
	and rax, ~(1 << 17); Clear PCID Flag
	mov cr4, rax
	ret

