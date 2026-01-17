#!/bin/bash
# Create bootable hdd.img with proper structure for Chrysalis OS
# Windows-like installation: installer.iso sets up hdd.img to be bootable

HDD_FILE="hdd.img"
HDD_SIZE=$((1024 * 1024 * 1024))  # 1GB in bytes

echo "Creating bootable hdd.img (1GB)..."

# Create 1GB raw image file
dd if=/dev/zero of=$HDD_FILE bs=1M count=1024 status=none 2>&1

if [ ! -f "$HDD_FILE" ]; then
    echo "✗ Failed to create hdd.img"
    exit 1
fi

echo "✓ Image file created (1GB)"

# Format with MBR + FAT32 partition
# Using parted or fdisk to create partition table
(
    echo "o"      # Create DOS partition table
    echo "n"      # New partition
    echo "p"      # Primary
    echo "1"      # Partition 1
    echo ""       # First sector (default)
    echo ""       # Last sector (default - use all)
    echo "t"      # Change type
    echo "c"      # FAT32
    echo "a"      # Mark bootable
    echo "1"      # Partition 1
    echo "w"      # Write
) | fdisk $HDD_FILE > /dev/null 2>&1

echo "✓ Partition table created (MBR + FAT32)"

# Format partition as FAT32 (write at offset 2048*512)
mkfs.fat -F 32 -n "CHRYSALIS" $HDD_FILE > /dev/null 2>&1

echo "✓ FAT32 filesystem created"

# Install GRUB into MBR
if command -v grub-install &> /dev/null; then
    LOOP_DEV=$(sudo losetup -f 2>/dev/null)
    if [ -n "$LOOP_DEV" ]; then
        sudo losetup $LOOP_DEV $HDD_FILE 2>/dev/null && \
        sudo grub-install --target=i386-pc --no-floppy --force \
            -d /usr/lib/grub/i386-pc $LOOP_DEV 2>/dev/null
        sudo losetup -d $LOOP_DEV 2>/dev/null
        echo "✓ GRUB bootloader installed"
    fi
else
    echo "⚠ grub-install not available"
fi

echo ""
echo "═════════════════════════════════════════════"
echo "hdd.img ready for Chrysalis OS installation"
echo "═════════════════════════════════════════════"
echo ""
echo "Next steps:"
echo "  1. npm run installer    - Run installer from ISO"
echo "  2. npm run boot         - Boot installed system"
echo ""
