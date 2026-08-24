/*
 * esp32_crt0.S — ESP32 Xtensa Microcontroller Startup Code Assembly
 * Sulfur++ AOT LLVM Compiler Toolchain
 */

.file "esp32_crt0.S"
.global call_start_cpu0
.global main

.section .iram0.text, "ax"
.align 4
.literal_position

call_start_cpu0:
    /* Initialize Xtensa Windowed Register Window (Window Base & Window Start) */
    entry sp, 32
    
    /* Set PS (Processor State) register: WOE (Window Overflow Enable) & INTLEVEL 0 */
    movi a2, 0x00040020
    wsr.ps a2
    rsync

    /* Clear BSS Memory Section in DRAM */
    movi a2, _bss_start
    movi a3, _bss_end
    movi a4, 0

bss_zero_loop:
    bgeu a2, a3, bss_zero_done
    s32i a4, a2, 0
    addi a2, a2, 4
    j bss_zero_loop

bss_zero_done:
    /* Call Sulfur++ Main Execution Entry Point */
    call8 main

    /* Execution halt fallback loop */
halt_loop:
    waiti 0
    j halt_loop
