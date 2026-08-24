/*
 * avr.ld — Linker script for 8-bit AVR Microcontroller Target (ATmega328P / Arduino Uno)
 * Sulfur++ AOT LLVM Compiler Toolchain
 */

OUTPUT_FORMAT("elf32-avr")
OUTPUT_ARCH(avr:5)
ENTRY(_start)

MEMORY
{
    flash (rx) : ORIGIN = 0x0000, LENGTH = 32K
    sram (rw!x) : ORIGIN = 0x800100, LENGTH = 2K
    eeprom (rw!x) : ORIGIN = 0x810000, LENGTH = 1K
}

SECTIONS
{
    /* Code Segment in Flash ROM */
    .text :
    {
        *(.vectors)
        *(.init0)
        *(.init2)
        *(.init4)
        *(.text)
        *(.text.*)
        *(.fini4)
        *(.fini0)
        _etext = .;
    } > flash

    /* Read-Only Data in Flash */
    .progmem.data :
    {
        *(.progmem*)
    } > flash

    /* Initialized Data (Copied from Flash to SRAM at startup) */
    .data : AT (_etext)
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.rodata)
        *(.rodata.*)
        _data_end = .;
    } > sram

    /* Uninitialized Data (Zeroed out at startup) */
    .bss (NOLOAD) :
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _bss_end = .;
    } > sram

    /* EEPROM Section */
    .eeprom :
    {
        *(.eeprom*)
    } > eeprom
}
