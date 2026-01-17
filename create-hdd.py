#!/usr/bin/env python3
"""Create bootable FAT32 hdd.img for Chrysalis OS"""

import os
import struct

HDD_FILE = "hdd.img"
HDD_SIZE = 1024 * 1024 * 1024

print("Creating bootable hdd.img (1GB FAT32)...")

# Create 1GB image
with open(HDD_FILE, "wb") as f:
    f.write(b'\x00' * HDD_SIZE)

print("OK: Image file created (1GB)")

# FAT32 boot sector
boot = bytearray(512)
boot[0:3] = b'\xeb\x3c\x90'
boot[3:11] = b'CHRYSCOS'
boot[11:13] = struct.pack('<H', 512)
boot[13] = 8
boot[14:16] = struct.pack('<H', 32)
boot[16] = 2
boot[17:19] = struct.pack('<H', 0)
boot[21] = 0xF8
boot[24:26] = struct.pack('<H', 63)
boot[26:28] = struct.pack('<H', 255)
boot[32:36] = struct.pack('<I', 2097152)
boot[36:40] = struct.pack('<I', 256)
boot[44:48] = struct.pack('<I', 2)
boot[48:50] = struct.pack('<H', 1)
boot[64] = 0x80
boot[66] = 0x29
boot[67:71] = struct.pack('<I', 0x12345678)
boot[71:82] = b'CHRYSALIS  '
boot[82:90] = b'FAT32   '
boot[510:512] = b'\x55\xaa'

with open(HDD_FILE, "r+b") as f:
    f.seek(0)
    f.write(bytes(boot))

print("OK: FAT32 boot sector written")

# FSInfo sector
fsinfo = bytearray(512)
fsinfo[0:4] = b'RRaA'
fsinfo[484:488] = b'rrAa'
fsinfo[510:512] = b'\x55\xaa'

with open(HDD_FILE, "r+b") as f:
    f.seek(512)
    f.write(bytes(fsinfo))

print("OK: FSInfo sector written")

# FAT tables
fat = bytearray(512 * 256 * 2)
fat[0:4] = b'\xf8\xff\xff\xff'
fat[4:8] = b'\xff\xff\xff\xff'

with open(HDD_FILE, "r+b") as f:
    f.seek(32 * 512)
    f.write(bytes(fat))

print("OK: FAT tables initialized")

# Root directory
root = bytearray(512 * 8)

with open(HDD_FILE, "r+b") as f:
    f.seek(2048 * 512)
    f.write(bytes(root))

print("OK: Root directory created")
print()
print("=" * 50)
print("hdd.img ready for Chrysalis OS")
print("=" * 50)
