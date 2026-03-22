global isr0
extern isr_handler

isr0:
    cli
    push 0          ; error code
    push 0          ; interrupt number
    jmp isr_common

isr_common:
    pusha
    call isr_handler
    popa
    add esp, 8
    sti
    iret