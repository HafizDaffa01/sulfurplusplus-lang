/*
 * esp32.ld — Linker script for ESP32 Xtensa Dual-Core Microcontroller Target
 * Sulfur++ AOT LLVM Compiler Toolchain
 */

ENTRY(call_start_cpu0)

MEMORY
{
    /* Internal Instruction RAM */
    iram0_0_seg (RX) : ORIGIN = 0x40080000, LENGTH = 0x20000     /* 128KB IRAM */
    
    /* Internal Data RAM */
    dram0_0_seg (RW) : ORIGIN = 0x3FFB0000, LENGTH = 0x2C000     /* 176KB DRAM */
    
    /* External Flash Instruction (SPI Flash Cache) */
    irom0_0_seg (RX) : ORIGIN = 0x400D0000, LENGTH = 0x330000    /* ~3.2MB Flash Text */
    
    /* External Flash Read-Only Data */
    drom0_0_seg (R)  : ORIGIN = 0x3F400000, LENGTH = 0x400000    /* 4MB Flash Read-Only Data */
}

SECTIONS
{
    /* Flash Read-Only Data (Constants, String Literals, Lookup Tables) */
    .flash.rodata :
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.srodata.*)
        _rodata_end = .;
    } > drom0_0_seg

    /* Flash Instruction Segment (AOT Compiled Functions) */
    .flash.text :
    {
        _stext = .;
        *(.literal .text .literal.* .text.*)
        _etext = .;
    } > irom0_0_seg

    /* Critical Fast RAM Functions (Interpreters, ISRs, Real-time loops) */
    .iram0.text :
    {
        _iram_start = .;
        *(.iram1 .iram1.*)
        *(.iram0.literal .iram0.text)
        _iram_end = .;
    } > iram0_0_seg

    /* Data Segment (Initialized Globals and Static Variables) */
    .dram0.data :
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    } > dram0_0_seg

    /* BSS Segment (Uninitialized Globals) */
    .dram0.bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } > dram0_0_seg

    /* Heap & Stack Boundaries */
    .heap_and_stack :
    {
        _heap_start = .;
    } > dram0_0_seg
}
