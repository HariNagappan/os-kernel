; gdt_flush.asm
; ─────────────────────────────────────────────────────────────────────
; PURPOSE:
;   Load our new GDT pointer into the GDTR hardware register, then
;   force the CPU to use the new descriptors by reloading every
;   segment register.
;
; WHY ASSEMBLY?
;   The 'far jump' (jmp 0x08:next) cannot be expressed in C.
;   It atomically loads a new CS value and flushes the prefetch
;   queue — the only safe way to switch code segment selectors.
;
; CALLING CONVENTION (cdecl):
;   Argument passed on stack at [esp+4].
;   No return value.
; ─────────────────────────────────────────────────────────────────────

[bits 32]
global gdt_flush   ; visible to C linker

gdt_flush:
    mov  eax, [esp+4]   ; EAX = address of gdt_ptr_t (passed from C)
    lgdt [eax]          ; Load 6 bytes into GDTR:
                        ;   GDTR.limit ← gdt_ptr.limit
                        ;   GDTR.base  ← gdt_ptr.base

    ; Reload CS (code segment) via a far jump.
    ; 0x08 = selector for GDT[1] (kernel code, ring 0).
    ; After lgdt, CS still holds the old value from BIOS/bootloader.
    ; The far jump forces the CPU to re-read the descriptor.
    jmp 0x08:.reload_cs

.reload_cs:
    ; Reload all data segment registers with kernel data selector.
    ; 0x10 = selector for GDT[2] (kernel data, ring 0).
    ; We use AX as the intermediary because segment registers
    ; cannot be loaded with an immediate value directly.
    mov  ax, 0x10
    mov  ds, ax   ; Data segment
    mov  es, ax   ; Extra segment
    mov  fs, ax   ; F segment (often used for TLS or special purposes)
    mov  gs, ax   ; G segment
    mov  ss, ax   ; Stack segment — must match privilege level
    ret