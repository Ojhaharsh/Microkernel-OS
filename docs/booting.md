# Week 1 — Bootstrapping (Bootloader & Kernel Entry — Long mode)

This milestone gets us from GRUB into 64-bit long mode and prints "Hello, Kernel".

## What happens at boot

- GRUB loads our ELF kernel into 32-bit protected mode and passes control to `_start`.
- `arch/x86_64/boot.S` does:
  - Load a minimal GDT with 64-bit code (0x08) and data (0x10) segments.
  - Enable PAE (CR4.PAE) and load CR3 with our PML4 address.
  - Enable long mode via `IA32_EFER.LME` and turn on paging (CR0.PG).
  - Perform a far jump to the 64-bit code segment (`long_mode_entry`).
  - Set a 64-bit stack and `call kernel_main`.
- `kernel/main.c` prints to VGA text buffer and writes a couple of messages over COM1 (serial), which shows in the terminal when QEMU is run with `-serial stdio`.

## Minimal paging

For Week 1 we identity-map the first 2 MiB using a single 2 MiB page:
- PML4[0] -> PDPT[0] -> PD[0] (PS=1 huge page)
- Flags: present | writable | page size (for PD entry)
- This is enough to run the entry and reach `kernel_main`.

## How to run (WSL)

```
# From the project root
scripts/run-wsl.sh
```

You can pass additional QEMU flags by adding `--`:

```
scripts/run-wsl.sh -- -no-reboot -d guest_errors
```

Expected:
- GRUB menu appears. Press Enter on "microkernel".
- QEMU display shows "Hello, Kernel" at top-left.
- The terminal shows serial logs like:
  - `[kernel] entered long mode`
  - `[kernel] printed to VGA`

## Troubleshooting

- "no multiboot header found": ensure `grub.cfg` uses `multiboot2 /boot/kernel.elf` (we include both MB1 and MB2 headers).
- If the screen stays blank:
  - Check the serial output in the terminal for the two kernel messages.
  - Rebuild from scratch: `make clean && make iso CROSS= && make run CROSS=`
  - Make sure you’re running under QEMU x86_64.
- If build fails due to missing cross-compiler:
  - The Makefile supports using system GCC by passing `CROSS=`.
  - For a full OSDev toolchain, install `x86_64-elf-gcc` (optional at this stage).

## Next (Week 2)

- Build real page tables (identity or higher-half mapping) beyond 2 MiB.
- Implement a physical page-frame allocator (bitmap) and a tiny `kmalloc`.
- Switch fully to higher-half kernel if desired.
