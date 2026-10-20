/* Script de link do autoteste MIPS: imagem plana a partir de 0x00400000 (KUSEG: o Unicorn traduz KSEG0). */
ENTRY(bkr_mips_selftest)
SECTIONS {
    . = 0x00400000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.MIPS.abiflags) *(.reginfo) *(.pdr) *(.comment) *(.gnu.attributes) }
}
