#include <stdint.h>

void page_fault_handler()
{
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    // print fault address (use your printk)
    while (1)
        ;
}