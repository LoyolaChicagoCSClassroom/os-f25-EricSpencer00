#!/usr/bin/env bash
set -euo pipefail

# Cleanup function: kill qemu (screen session or background pid)
cleanup() {
  if [ -n "${QEMU_SCREEN:-}" ]; then
    if command -v screen >/dev/null 2>&1; then
      screen -S "$QEMU_SCREEN" -X quit || true
    fi
  fi
  if [ -n "${QEMU_PID:-}" ]; then
    kill "$QEMU_PID" 2>/dev/null || true
  fi
}

trap cleanup EXIT

# Detect boot medium: prefer grub.iso, then rootfs.img
if [ -f grub.iso ]; then
  QEMU_IMAGE_TYPE=iso
  QEMU_IMAGE=grub.iso
  QEMU_ARGS=( -cdrom "$QEMU_IMAGE" -boot d -m 512 )
elif [ -f rootfs.img ]; then
  QEMU_IMAGE_TYPE=img
  QEMU_IMAGE=rootfs.img
  QEMU_ARGS=( -hda "$QEMU_IMAGE" -m 512 )
else
  echo "Error: no grub.iso or rootfs.img found. Build one with 'make iso' or 'make rootfs.img'" >&2
  exit 1
fi

# QEMU base command (pause and listen for gdb on :1234)
QEMU_CMD=( qemu-system-i386 -S -s "${QEMU_ARGS[@]}" )

echo "Starting QEMU ($QEMU_IMAGE_TYPE -> $QEMU_IMAGE)"

# If screen is available, launch QEMU in a detached screen session so serial stdio goes to the screen window
if command -v screen >/dev/null 2>&1; then
  QEMU_SCREEN="qemu"
  # merge -serial stdio so you can attach the screen session to see serial output
  screen -dmS "$QEMU_SCREEN" bash -lc "${QEMU_CMD[*]} -serial stdio"
  echo "QEMU started in detached screen session '$QEMU_SCREEN'. Attach with: screen -r $QEMU_SCREEN"
else
  # Fallback: start QEMU in background and expose serial via TCP
  QEMU_PORT=4444
  bash -c "${QEMU_CMD[*]} -serial tcp:127.0.0.1:${QEMU_PORT},server,nowait &" 
  QEMU_PID=$!
  echo "QEMU started in background (pid=$QEMU_PID). Serial available on TCP port $QEMU_PORT (use: nc 127.0.0.1 $QEMU_PORT)"
fi

# Choose gdb binary (prefer cross-gdb if installed)
if command -v i686-elf-gdb >/dev/null 2>&1; then
  GDB=i686-elf-gdb
elif command -v gdb-multiarch >/dev/null 2>&1; then
  GDB=gdb-multiarch
else
  GDB=gdb
fi

echo "Starting GDB ($GDB) and attaching to QEMU..."
TERM=xterm "$GDB" -x gdb_os.txt

# GDB exited; cleanup will run via trap
echo "GDB exited. Cleaning up QEMU."


