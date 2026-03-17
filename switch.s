; This function is called by your C scheduler
; void context_switch(pcb_t* next_process)

global context_switch
context_switch:
    ; 1. Get the pointer to the next PCB from the stack
    mov eax, [esp + 4] 

    ; 2. Load the saved registers from that PCB into the CPU
    mov ebx, [eax + 12] ; Offset for EBX in our pcb_t
    mov ecx, [eax + 16] ; Offset for ECX
    mov edx, [eax + 20] ; Offset for EDX
    
    ; 3. The Big Jump: Load the Instruction Pointer
    ; In a real kernel, we'd use 'iret', but for a basic project:
    push dword [eax + 32] ; Push the saved EIP
    ret                    ; "Return" to the new process's code!