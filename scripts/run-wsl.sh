#!/usr/bin/env bash
set -euo pipefail

# Resolve project root as parent of this script
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
cd "${ROOT_DIR}"

# Allow passing extra QEMU flags: scripts/run-wsl.sh -- -no-reboot -d guest_errors
EXTRA_QEMU_FLAGS=()
if [[ $# -gt 0 && "$1" == "--" ]]; then
  shift
  EXTRA_QEMU_FLAGS=("$@")
fi

# Build with system gcc (CROSS= empty)
make clean
make iso CROSS=
# If extra QEMU flags provided, propagate via env var
if [[ ${#EXTRA_QEMU_FLAGS[@]} -gt 0 ]]; then
  QEMU_FLAGS="${EXTRA_QEMU_FLAGS[*]}" make run CROSS=
else
  make run CROSS=
fi
