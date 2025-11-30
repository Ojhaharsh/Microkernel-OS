#!/usr/bin/env bash
set -euo pipefail

# Resolve project root as parent of this script
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
cd "${ROOT_DIR}"

MODE=headless
EXTRA_QEMU_FLAGS=()

# Usage examples:
#   scripts/run-wsl.sh                    # headless (serial only)
#   scripts/run-wsl.sh --gui              # GUI + serial
#   scripts/run-wsl.sh -- --no-reboot     # headless with extra QEMU flags
#   scripts/run-wsl.sh --gui -- -d guest_errors

while [[ $# -gt 0 ]]; do
  case "$1" in
    --gui)
      MODE=gui
      shift
      ;;
    --)
      shift
      EXTRA_QEMU_FLAGS=("$@")
      break
      ;;
    *)
      # Ignore unknown args
      shift
      ;;
  esac
done

# Build with system gcc (CROSS= empty)
make clean
make iso CROSS=

# Choose run target and propagate QEMU_FLAGS if provided
if [[ "$MODE" == "gui" ]]; then
  if [[ ${#EXTRA_QEMU_FLAGS[@]} -gt 0 ]]; then
    QEMU_FLAGS="${EXTRA_QEMU_FLAGS[*]}" make run CROSS=
  else
    make run CROSS=
  fi
else
  if [[ ${#EXTRA_QEMU_FLAGS[@]} -gt 0 ]]; then
    QEMU_FLAGS="${EXTRA_QEMU_FLAGS[*]}" make run-headless CROSS=
  else
    make run-headless CROSS=
  fi
fi
