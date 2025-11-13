# Toolchain (use a cross-compiler if available)
CROSS ?= x86_64-elf-
CC := $(CROSS)gcc
LD := $(CROSS)ld
AS := $(CROSS)gcc

BUILD := build
ISO_DIR := $(BUILD)/iso
GRUB_DIR := $(ISO_DIR)/boot/grub
KERNEL := $(BUILD)/kernel.elf
ISO := $(BUILD)/microkernel.iso

CFLAGS := -ffreestanding -O2 -Wall -Wextra -m64 -fno-stack-protector -fno-pic -fno-pie -fno-omit-frame-pointer -Iinclude
LDFLAGS := -nostdlib -z max-page-size=0x1000

OBJS := \
	arch/x86_64/boot.o \
	arch/x86_64/context_switch.o \
	arch/x86_64/interrupt.o \
	arch/x86_64/syscall_entry.o \
	arch/x86_64/gdt.o \
	arch/x86_64/user.o \
	arch/x86_64/user_sysret.o \
	arch/x86_64/userprog.o \
	kernel/scheduler.o \
	kernel/interrupts.o \
	kernel/syscall.o \
	kernel/main.o \
	kernel/memory.o

.PHONY: all clean run iso qemu gdb

all: $(KERNEL)

$(BUILD):
	@mkdir -p $(BUILD)

arch/x86_64/boot.o: arch/x86_64/boot.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/context_switch.o: arch/x86_64/context_switch.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/interrupt.o: arch/x86_64/interrupt.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/syscall_entry.o: arch/x86_64/syscall_entry.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/gdt.o: arch/x86_64/gdt.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/user.o: arch/x86_64/user.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/userprog.o: arch/x86_64/userprog.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

arch/x86_64/user_sysret.o: arch/x86_64/user_sysret.S | $(BUILD)
	$(AS) -m64 -c $< -o $@

kernel/main.o: kernel/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): linker.ld $(OBJS) | $(BUILD)
	$(LD) -T $< -o $@ $(OBJS) $(LDFLAGS)

iso: $(ISO)

$(ISO): $(KERNEL)
	@echo "[ISO] Creating GRUB ISO image"
	@mkdir -p $(GRUB_DIR)
	cp $(KERNEL) $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio

qemu: all
	# Direct boot via -kernel is not universally reliable for Multiboot; prefer ISO.
	qemu-system-x86_64 -kernel $(KERNEL) -serial stdio

gdb: iso
	qemu-system-x86_64 -cdrom $(ISO) -s -S

clean:
	rm -rf $(BUILD) $(OBJS)
