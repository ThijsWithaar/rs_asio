.section .text

# The assembly from the Microsoft assembler was checked with
#   dumpbin /disasm NtProtectVirtualMemory.obj
# The gcc assembly checked with:
#   objdump -d bld/aobj/NtProtectVirtualMemory.o
# To get a match, the call and jump instructions have been replaced by their immediate values

.global NtProtectVirtualMemory
.type NtProtectVirtualMemory, @function

NtProtectVirtualMemory:
    movl $0x50, %eax            # B8 50 00 00 00     mov         eax,50h
#    call KiSystemCall          # E8 01 00 00 00     call        _KiSystemCall
    .word 0x01E8
    .word 0x0000
    .byte 0x00
    ret                         # C3
.size NtProtectVirtualMemory, .-NtProtectVirtualMemory


.global KiSystemCall
.type KiSystemCall, @function

KiSystemCall:
#ifdef _WIN64
    mov %rcx, %r10
    syscall
    ret
#else
    .byte 0xea              # EA 00 00 00 00 33  jmp         exit
    .long 0x00              #exit
    .word 0x33
#endif
exit:
    incl %ecx                # 41                 inc         ecx
    jmp *0xf8(%edi)          # FF A7 F8 00 00 00  jmp         dword ptr [edi+0F8h]
.size KiSystemCall, .-KiSystemCall
