/*
 * crt0.S — Generic / AVR C Runtime Startup Assembly
 * Sulfur++ AOT LLVM Compiler Toolchain
 */

.file "crt0.S"
.global _start
.global main

.section .vectors, "ax", @progbits
_start:
    /* Interrupt vector table entry: Reset Jump */
    rjmp init_system

.section .init2, "ax", @progbits
init_system:
    /* Clear SREG */
    eor r1, r1
    out 0x3f, r1

    /* Set Stack Pointer to RAMEND (0x08FF for ATmega328P) */
    ldi r28, 0xFF
    ldi r29, 0x08
    out 0x3e, r29
    out 0x3d, r28

.section .init4, "ax", @progbits
zero_bss:
    /* Zero out .bss memory region */
    ldi r30, lo8(_bss_start)
    ldi r31, hi8(_bss_start)
    ldi r24, lo8(_bss_end)
    ldi r25, hi8(_bss_end)

zero_bss_loop:
    cp r30, r24
    cpc r31, r25
    brcc copy_data
    st Z+, r1
    rjmp zero_bss_loop

copy_data:
    /* Copy .data segment from Flash to SRAM */
    ldi r30, lo8(_etext)
    ldi r31, hi8(_etext)
    ldi r26, lo8(_data_start)
    ldi r27, hi8(_data_start)
    ldi r24, lo8(_data_end)
    ldi r25, hi8(_data_end)

copy_data_loop:
    cp r26, r24
    cpc r27, r25
    brcc call_main
    lpm r0, Z+
    st X+, r0
    rjmp copy_data_loop

call_main:
    /* Jump into Sulfur++ main entry point */
    call main

exit_loop:
    cli
halt:
    rjmp halt
