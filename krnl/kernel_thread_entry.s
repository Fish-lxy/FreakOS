[EXTERN do_exit]
[GLOBAL kernel_thread_entry_s]
kernel_thread_entry_s:
    push edx;
    call ebx;

    push eax
    call do_exit