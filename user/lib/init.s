[EXTERN umain]
[SECTION .text]
[GLOBAL user_start]
user_start:
    ; set ebp for backtrace
    mov ebp, 0

    ; load argc and argv
    mov ebx, [esp]
    lea ecx, [esp+4]

    ; move down the esp register
    ; since it may cause page fault in backtrace
    sub esp, 0x20

    ; save argc and argv on stack
    push ecx
    push ebx

    ; call user-program function
    call umain
l1: 
    jmp l1