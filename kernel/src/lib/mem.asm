; global memset
; global memset16
; global memset32
; global memset64

memset:
    ret
    mov rcx, rdx
    mov rax, rsi
    rep stosb
    ret