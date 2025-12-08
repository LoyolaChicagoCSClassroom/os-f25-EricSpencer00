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
# Rebuild the ISO by default so the launched QEMU runs the latest build.
# Set SKIP_BUILD=1 in the environment to skip this step.
if [ -z "${SKIP_BUILD:-}" ]; then
  echo "Building grub.iso (set SKIP_BUILD=1 to skip)..."
  make iso
fi

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

# Detect available QEMU binary
if command -v qemu-system-x86_64 >/dev/null 2>&1; then
  QEMU_BIN=qemu-system-x86_64
elif command -v qemu-system-i386 >/dev/null 2>&1; then
  QEMU_BIN=qemu-system-i386
else
  echo "Error: no qemu system binary found (qemu-system-x86_64 or qemu-system-i386)." >&2
  echo "On macOS M1, install QEMU via Homebrew:  brew install qemu" >&2
  exit 127
fi

# QEMU base command. By default run without GDB. Set GDB=1 to enable debugging.
if [ "${GDB:-}" = "1" ]; then
  # Pause QEMU and listen for gdb on :1234
  QEMU_CMD=( "$QEMU_BIN" -S -s "${QEMU_ARGS[@]}" )
  USE_GDB=true
else
  # Run normally without gdb stub (default)
  QEMU_CMD=( "$QEMU_BIN" "${QEMU_ARGS[@]}" )
  USE_GDB=false
fi

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

if [ "$USE_GDB" = "true" ]; then
  echo "Starting GDB ($GDB) and attaching to QEMU..."
  TERM=xterm "$GDB" -x gdb_os.txt

  # GDB exited; cleanup will run via trap
  echo "GDB exited. Cleaning up QEMU."
else
  echo "Kernel running in QEMU without GDB (use GDB=1 to debug)."
  echo "Press Ctrl+C to stop QEMU."
fi


