[EXTERN do_exit]
[GLOBAL kernel_thread_entry_s]
kernel_thread_entry_s:
    push edx;
    call ebx;

    push eax
    call do_exit

[GLOBAL fork_return_s]
fork_return_s:
    mov esp, [esp+4]
    jmp _interrupt_ret
_interrupt_ret:
    pop gs	    ; 恢复原来的段描述符
	pop fs
	pop es
	pop ds
	popa        ; 弹出 edi, esi, ebp, esp, ebx, edx, ecx, eax
	add esp, 8  ; 清理栈里的 error code 和 ISR
	iret

[GLOBAL switch_to_s]
switch_to_s:            ;switch_to(from,to)
    mov eax, [esp+4]
    pop dword [eax]

    mov [eax+4], esp
    mov [eax+8], ebx
    mov [eax+12], ecx
    mov [eax+16], edx
    mov [eax+20], esi
    mov [eax+24], edi
    mov [eax+28], ebp

    mov eax, [esp+4]

    mov ebp, [eax+28]
    mov edi, [eax+24]
    mov esi, [eax+20]
    mov edx, [eax+16]
    mov ecx, [eax+12]
    mov ebx, [eax+8]
    mov esp, [eax+4]

    push dword [eax]
    ret


