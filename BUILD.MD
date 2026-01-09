# Chrysalis OS – Build Guide

This document describes how to build **Chrysalis OS** from source.

Chrysalis OS is designed to be built on a Linux host using a cross-compiler.
Native compilation is **not supported**.

---

## Supported Host Systems

Recommended:
- Linux (Debian / Ubuntu / Arch / Fedora)

Not supported:
- Windows (without WSL)
- macOS (untested)

---

## Required Dependencies

### Core build tools

Install the following packages:

```sh
sudo apt install build-essential gcc-multilib g++-multilib \
                 nasm make xorriso grub-pc-bin \
                 qemu-system-i386
````

---

### Cross Compiler (Recommended)

Although `gcc -m32` works, a dedicated cross-compiler is **strongly recommended**
for stability and correctness.

Target:

```
i686-elf
```

Required binaries:

* `i686-elf-gcc`
* `i686-elf-ld`
* `i686-elf-objcopy`

---

## Directory Structure

Key directories:

```
.
├── kernel/
├── arch/
├── build/
├── iso/
├── tools/
├── Makefile
```

---

## Build Output

The build process generates:

* Kernel ELF binary
* Bootable ISO image
* Optional raw disk image

---

## Building the Kernel

From the project root:

```sh
make
```

This will:

* Compile all kernel sources
* Assemble boot code
* Link the kernel
* Produce a bootable ISO

---

## Running in QEMU

Run the OS using:

```sh
make run
```

Or manually:

```sh
qemu-system-i386 \
  -cdrom chrysalis.iso \
  -serial stdio \
  -m 128M
```

---

## Debug Builds

To enable debug symbols:

```sh
make DEBUG=1
```

Recommended for kernel development.

---

## Cleaning the Build

Remove all build artifacts:

```sh
make clean
```

---

## Common Build Issues

### `undefined reference to __udivdi3`

Cause:

* 64-bit division used in freestanding kernel code.

Fix:

* Avoid 64-bit division
* Or provide a manual implementation

---

### Kernel boots but hangs

Tips:

* Enable serial logging early
* Verify interrupts are acknowledged (EOI)
* Check paging mappings

---

### Framebuffer crashes QEMU

Cause:

* Framebuffer memory not fully mapped

Fix:

* Map full `height × pitch` bytes
* Align mappings to page boundaries

---

## Notes for Modders & Distros

Chrysalis OS uses the **MIT License**.

You are free to:

* Modify the build system
* Create custom ISOs
* Distribute downstream builds

Attribution is required.

---

## Reproducible Builds

Chrysalis OS does not currently guarantee reproducible builds.
Toolchain versions may affect output.

---

End of file

