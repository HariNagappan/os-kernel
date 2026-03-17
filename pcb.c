#include "pcb.h"
#include "kmalloc.h"

static uint32_t next_pid = 1;

pcb_t* create_process(uint32_t entry_point) {

    pcb_t* new_pcb = (pcb_t*)kmalloc(sizeof(pcb_t));
    
    if (new_pcb == NULL) return NULL;

    new_pcb->pid = next_pid++;
    new_pcb->state = READY;
    
    // Set the instruction pointer so the CPU knows where to start
    new_pcb->eip = entry_point;
    
    // In a real kernel, you'd also allocate a separate stack here
    // For now, we just initialize the pointers to 0
    new_pcb->esp = 0; 
    new_pcb->ebp = 0;
    new_pcb->eflags = 0x202; // Standard "interrupts enabled" flag for x86
    
    new_pcb->next = NULL;
    
    return new_pcb;
}