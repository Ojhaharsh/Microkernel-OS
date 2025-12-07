#!/bin/bash

# Simple script to run QEMU and capture output using script command
# Resolve project root dynamically
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
cd "${SCRIPT_DIR}"

echo "Building ISO..."
make iso CROSS=

echo "Running QEMU and capturing output..."
# Use script to capture session
script -q -c "timeout 8s qemu-system-x86_64 -cdrom build/microkernel.iso -serial stdio -display none" /tmp/qemu-session.log

echo "=== CAPTURED OUTPUT ==="
cat /tmp/qemu-session.log
echo "=== END OUTPUT ==="

# Copy to build directory
cp /tmp/qemu-session.log "${SCRIPT_DIR}/build/serial.log"
echo "Output saved to build/serial.log"