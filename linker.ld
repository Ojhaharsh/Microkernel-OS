OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
  /* Load the kernel at 1 MiB (typical GRUB load address) */
  . = 0x00100000;

  /* Ensure the multiboot header is within the first 8 KiB */
  .multiboot : ALIGN(8) {
    KEEP(*(.multiboot))
  }

  .text : ALIGN(4K) {
    KEEP(*(.text.boot))
    *(.text*)
  }

  .rodata : ALIGN(4K) {
    *(.rodata*)
  }

  .data : ALIGN(4K) {
    *(.data*)
  }

  .bss : ALIGN(4K) {
    *(.bss*)
    *(COMMON)
  }
}
