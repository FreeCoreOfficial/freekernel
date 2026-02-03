# CHRYSALIS OS - DEPLOYMENT GUIDE

## ✅ SYSTEM STATUS: PRE-ALPHA

**Component:** Freestanding Multiboot Kernel
**Bootloader:** GRUB 2 (multiboot compliant)
**Target Platform:** x86 32-bit systems (PC/laptop/VM)

---

## INSTALLATION METHODS

### Method 1: QEMU Virtual Machine (Recommended)

**Requirements:**

- QEMU installed (`sudo apt install qemu-system-x86`)
- Development environment setup (see README.md)

**Steps:**

```bash
# 1. Navigate to the os directory
cd os

# 2. Build the system
make

# 3. Run QEMU
make run
```

### Method 2: Bootable ISO (Real Hardware/VirtualBox)

**Requirements:**

- `grub-mkrescue`
- `xorriso`

**Steps:**

```bash
# 1. Build the ISO
cd os
make iso

# 2. The ISO will be generated at:
#    ./chrysalis.iso
```

**Deploying to USB:**

```bash
# Write to USB (Replace /dev/sdX with your USB drive)
sudo dd if=chrysalis.iso of=/dev/sdX bs=4M status=progress && sync
```

---

## BOOT PROCESS

1. **BIOS/UEFI** initializes.
2. **GRUB bootloader** loads.
3. **Chrysalis Kernel** is loaded into memory via Multiboot protocol.
4. **Kernel Entry** point reached (`kernel_main`).
5. **Subsystems Initialize** (GDT, IDT, ISR, IRQ, Keyboard, etc.).
6. **Shell/Terminal** becomes active.

---

## TROUBLESHOOTING

**Problem:** "Multiboot header not found"

- **Solution:** Ensure the kernel is compiled with the multiboot header within the first 8KiB.

**Problem:** QEMU crashes immediately

- **Solution:** Check serial output/logs if enabled. Ensure you are ensuring 32-bit mode (`qemu-system-i386`).

---

## SUPPORT RESOURCES

- **OSDev Wiki:** <https://wiki.osdev.org/>
- **Chrysalis OS:** Internal documentation
