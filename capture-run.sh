#!/bin/bash

# Simple script to run QEMU and capture output using script command
cd '/mnt/c/Users/KIIT/Desktop/Microkernel OS'

echo "Building ISO..."
make iso CROSS=

echo "Running QEMU and capturing output..."
# Use script to capture session
script -q -c "timeout 8s qemu-system-x86_64 -cdrom build/microkernel.iso -serial stdio -display none" /tmp/qemu-session.log

echo "=== CAPTURED OUTPUT ==="
cat /tmp/qemu-session.log
echo "=== END OUTPUT ==="

# Copy to Windows location
cp /tmp/qemu-session.log '/mnt/c/Users/KIIT/Desktop/Microkernel OS/build/serial.log'
echo "Output saved to build/serial.log"