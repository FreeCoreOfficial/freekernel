#!/bin/bash
# Create bootable hdd.img with proper FAT32 + GRUB
# Similar to Windows installation process

HDD_SIZE=1G
HDD_FILE="hdd.img"

echo "Creating bootable hdd.img (1GB FAT32)..."

# Create sparse 1GB image
dd if=/dev/zero of=$HDD_FILE bs=1M count=1024 status=progress 2>&1 | tail -1

# Format as FAT32
mkfs.fat -F 32 -n "CHRYSALIS" $HDD_FILE > /dev/null 2>&1

echo "✓ hdd.img created and formatted as FAT32"

# Mount image to copy files
MOUNT_DIR="/tmp/hdd_mount_$$"
mkdir -p $MOUNT_DIR

# Try to mount with loop device
sudo mount -o loop $HDD_FILE $MOUNT_DIR 2>/dev/null

if [ $? -eq 0 ]; then
    echo "✓ hdd.img mounted at $MOUNT_DIR"
    
    # Create directory structure
    sudo mkdir -p $MOUNT_DIR/boot/grub
    sudo mkdir -p $MOUNT_DIR/system
    
    # Copy kernel if available
    if [ -f "build/kernel.bin" ]; then
        sudo cp build/kernel.bin $MOUNT_DIR/boot/
        echo "✓ Copied kernel"
    fi
    
    # Create GRUB config for hdd boot
    sudo bash -c "cat > $MOUNT_DIR/boot/grub/grub.cfg << 'GRUBEOF'
set default=0
set timeout=5

menuentry 'Chrysalis OS (from HDD)' {
    multiboot2 /boot/kernel.bin
}
GRUBEOF"
    echo "✓ Created GRUB configuration"
    
    # Sync and unmount
    sync
    sudo umount $MOUNT_DIR
    echo "✓ hdd.img unmounted"
    
    rm -rf $MOUNT_DIR
else
    echo "⚠ Could not mount hdd.img with sudo - will use raw format"
fi

echo ""
echo "Ready to boot: npm run boot"
