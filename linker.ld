OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
  /* Load the kernel at 1 MiB (typical GRUB load address) */
  . = 0x00100000;

  _kernel_start = .;

  /* Ensure the multiboot header is at the very beginning */
  .multiboot : ALIGN(8) {
    KEEP(*(.multiboot))
    KEEP(*(.multiboot2))
  }

  .text : {
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

  _kernel_end = .;
}
