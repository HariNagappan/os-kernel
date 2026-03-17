; isr_stub.asm
; These are the raw entry points that the IDT points to.
; The CPU jumps here when an interrupt fires.
;
; Job: save all registers, call the C handler, restore registers, return.
; The struct layout here MUST match registers_t in isr.h.

[BITS 32]
[EXTERN isr_divide_by_zero]
[EXTERN isr_double_fault]
[EXTERN isr_general_protection]
[EXTERN isr_page_fault]
[EXTERN irq0_timer_handler]

[GLOBAL isr0_stub]
[GLOBAL isr8_stub]
[GLOBAL isr13_stub]
[GLOBAL isr14_stub]
[GLOBAL irq0_stub]

; ---------------------------------------------------------------
; Macro: exceptions WITH no error code pushed by CPU
;   We push a dummy 0 so the stack frame is always the same shape.
; ---------------------------------------------------------------
%macro ISR_NOERR 2          ; args: stub_name, int_number
%1:
    push dword 0            ; dummy error code
    push dword %2           ; interrupt number
    jmp  isr_common
%endmacro

; ---------------------------------------------------------------
; Macro: exceptions where the CPU already pushed an error code
; ---------------------------------------------------------------
%macro ISR_ERR 2            ; args: stub_name, int_number
%1:
    push dword %2           ; interrupt number (error code already on stack)
    jmp  isr_common
%endmacro

; ---------------------------------------------------------------
; Macro: hardware IRQ stubs (no error code, no CPU-pushed code)
; ---------------------------------------------------------------
%macro IRQ_STUB 2           ; args: stub_name, handler_name
%1:
    push dword 0            ; dummy error code
    push dword 0            ; dummy int number
    pusha
    push esp                ; pass pointer to registers_t
    call %2
    add  esp, 4
    popa
    add  esp, 8             ; pop dummy int_no + err_code
    iret
%endmacro

; ---------------------------------------------------------------
; Individual stubs
; ---------------------------------------------------------------
ISR_NOERR isr0_stub,  0    ; Divide By Zero
ISR_ERR   isr8_stub,  8    ; Double Fault       (CPU pushes error code)
ISR_ERR   isr13_stub, 13   ; General Protection (CPU pushes error code)
ISR_ERR   isr14_stub, 14   ; Page Fault         (CPU pushes error code)
IRQ_STUB  irq0_stub, irq0_timer_handler

; ---------------------------------------------------------------
; Common ISR path for CPU exceptions
; ---------------------------------------------------------------
isr_common:
    pusha                   ; push edi,esi,ebp,esp,ebx,edx,ecx,eax

    push esp                ; pass pointer to the full register frame as arg

    ; Pick the right C handler based on int_no
    ; (esp+4 is the saved esp from pusha, so int_no is at esp+36)
    mov  eax, [esp + 36]   ; int_no

    cmp  eax, 0
    je   .call_div_zero
    cmp  eax, 8
    je   .call_double_fault
    cmp  eax, 13
    je   .call_gpf
    cmp  eax, 14
    je   .call_page_fault
    jmp  .done             ; unhandled: just return

.call_div_zero:
    call isr_divide_by_zero
    jmp  .done
.call_double_fault:
    call isr_double_fault
    jmp  .done
.call_gpf:
    call isr_general_protection
    jmp  .done
.call_page_fault:
    call isr_page_fault

.done:
    add  esp, 4            ; pop the pointer we pushed before the call
    popa                   ; restore edi,esi,ebp,esp,ebx,edx,ecx,eax
    add  esp, 8            ; pop int_no and err_code
    iret                   ; restore eip, cs, eflags (and esp/ss in user mode)