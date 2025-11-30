.section .text


.global NtProtectVirtualMemory
.type NtProtectVirtualMemory, @function

NtProtectVirtualMemory:
    movl $0x50, %eax
    call KiSystemCall
    ret
.size NtProtectVirtualMemory, .-NtProtectVirtualMemory


.global KiSystemCall
.type KiSystemCall, @function

KiSystemCall:
#ifdef _WIN64
    mov %rcx, %r10
    syscall
    ret
#else
    .byte 0xea
    .long exit
    .word 0x33
#endif
exit:
    incl %ecx
    jmp *0xf8(%edi)
.size KiSystemCall, .-KiSystemCall
