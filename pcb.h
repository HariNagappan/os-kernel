#ifndef PCB_H
#define PCB_H

#include "types.h"

typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} process_state_t;

typedef struct pcb {
    uint32_t pid;
    process_state_t state;
    
    // CPU Context: We save the registers here during a switch
    uint32_t eax, ebx, ecx, edx; 
    uint32_t esi, edi;
    uint32_t esp;                // Stack Pointer
    uint32_t ebp;                // Base Pointer
    uint32_t eip;                // Instruction Pointer (Where it left off)
    uint32_t eflags;             // CPU status flags
    
    struct pcb* next;            // Next process in our Round Robin circle
} pcb_t;

pcb_t* create_process(uint32_t entry_point);

extern void context_switch(pcb_t* next_process);

#endif