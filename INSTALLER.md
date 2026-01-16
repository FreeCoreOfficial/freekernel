# Chrysalis OS System Installer Guide

## Overview

The Chrysalis OS installer (`installer` command) sets up persistent system configuration on the hard disk. It copies the icons library and system files from the bootloader to the disk, making the system work independently without requiring the ISO to be attached after first boot.

## Features

✅ **First-Run Installation**
- Detects bootloader modules (icons.mod) from GRUB multiboot
- Copies icons library to persistent FAT32 storage (/system/icons.mod)
- Creates system configuration files

✅ **Persistent Storage**
- Icons library stored at `/system/icons.mod` on disk
- System information stored at `/system/info.txt`
- Readme documentation at `/system/readme.txt`

✅ **Clean Separation**
- Installation data is separate from ISO
- System remains functional after reboot without ISO attached
- Compatible with existing file structure

## Usage

### First Boot with ISO

1. Boot Chrysalis OS with the ISO attached
2. System boots to shell prompt
3. Run installer command:
   ```
   > installer
   ```

4. Installer will:
   - Mount FAT32 filesystem
   - Load icons.mod from bootloader (if present)
   - Copy it to `/system/icons.mod`
   - Create system metadata files
   - Display status and completion

### Subsequent Boots

After first boot with installer:
1. Icons are loaded from `/system/icons.mod` on disk
2. System works completely independently
3. ISO attachment is no longer required
4. All GUI features work with persistent icons

## File Locations

- **Bootloader Module**: Detected at 0x1e2000 (multiboot location)
- **Persistent Icons**: `/system/icons.mod` (67 MB on disk)
- **System Info**: `/system/info.txt`
- **Readme**: `/system/readme.txt`

## Technical Details

### Icon Loading Priority

1. **Persistent (FAT32)**: `/system/icons.mod` - checked first
2. **Bootloader (RAMFS)**: Multiboot module - fallback only
3. **Graceful Degradation**: GUI runs without icons if loading fails

### Size Considerations

- icons.mod: 67 MB (16 x 1024x1024 RGBA8888 images)
- Ensure at least 100 MB free space on disk
- Total system needs ~200 MB for comfortable operation

### Disk Format

- Filesystem: FAT32
- Partition size: Configured during qemu disk setup
- Boot: GRUB multiboot2 compatible

## Troubleshooting

### Icons not displaying after installer
1. Verify disk has sufficient space:
   ```
   > fat
   ```
2. Check if icons.mod was written:
   ```
   > ls /system/
   ```
3. Run installer again if needed:
   ```
   > installer
   ```

### System won't boot without ISO
- Ensure installer was successfully completed
- Check `/system/info.txt` exists on disk
- Reinstall if necessary

## Manual Icon Installation

If automatic installation fails:

1. Boot with ISO attached
2. Mount FAT32:
   ```
   > fat automount
   ```

3. Verify source (icons.mod should be in bootloader):
   ```
   > ramfs list
   ```

4. Check destination:
   ```
   > ls /system/
   ```

## Recovery

To reset and reinstall:

1. Delete system files from FAT32 partition
2. Boot with ISO attached
3. Run `installer` again
4. System will reinitialize all settings

## File Structure After Installation

```
/system/
├── icons.mod           (67 MB - Icon library)
├── info.txt            (System metadata)
├── readme.txt          (Documentation)
└── pkg/                (Package directory)
```

## Performance

- First boot with installer: ~5-10 seconds for copying
- Subsequent boots: Icons load from disk (faster than multiboot)
- No performance degradation after installation
- System remains responsive during installation

## Version

Installer Version: 1.0  
Chrysalis OS Build: 2026-01-16  
Supported Icon Format: RGBA8888, 1024x1024
