global _start
extern kernel_main

section .text
_start:
    cli
    call kernel_main
    hlt
    