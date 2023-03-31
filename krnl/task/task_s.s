[EXTERN do_exit]
[GLOBAL kernel_thread_entry_s]
kernel_thread_entry_s:
    push edx;   ; 压入参数arg
    call ebx;   ; 调用func

    push eax    ; 压入func返回值
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



; 进入函数之后,从上到下的栈内参数为: &(next->context), &(prev->context), 返回地址.
[GLOBAL switch_to_s]
switch_to_s:            ; switch_to(from,to)
    ; 在栈上保存 from 的进程上下文
    mov eax, [esp+4]    ; eax 指向 from,其实也指向了 from 的 eip
    pop dword [eax] ; pop 前,esp 指向调用者的 eip,即返回地址,pop 后直接把 eip 赋给了 from 的 eip 变量. 然后 esp 指向了 from.

    ; 把当前各种寄存器的值保存在 from context 中.
    mov [eax+4], esp
    mov [eax+8], ebx
    mov [eax+12], ecx
    mov [eax+16], edx
    mov [eax+20], esi
    mov [eax+24], edi
    mov [eax+28], ebp

    ; 把 to 中的值赋给寄存器
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


