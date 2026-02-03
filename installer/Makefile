# Minimal Chrysalis OS Installer Makefile
# Purpose: Build installer ISO with setup wizard

CC = gcc
CXX = g++
AS = nasm
LD = ld

CFLAGS = -fno-builtin -Wall -Wextra -ffreestanding -O2 -m32 -I../os/kernel/include
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = -f elf32
LDFLAGS = -nostdlib -m elf_i386 -T ../os/linker.ld

BUILD = build
ISO = iso

# Minimal object files (installer only)
# Minimal object files + OS Drivers
OBJS = \
	$(BUILD)/multiboot2_header.o \
	$(BUILD)/boot.o \
	$(BUILD)/kernel_installer.o \
	$(BUILD)/installer.o \
	$(BUILD)/stubs.o \
	../os/build/stdlib_wrappers.o \
	../os/build/disk.o \
	../os/build/ata.o \
	../os/build/fat_fs.o \
	../os/build/cmd_fat.o \
	../os/build/pmm.o \
	../os/build/serial.o \
	../os/build/ramfs.o \
	../os/build/mem/kmalloc_init.o \
	../os/build/mem/kmalloc.o \
	../os/build/string.o

# Force looking for headers in OS include
CFLAGS += -I../os/kernel/include

all: $(BUILD) installer.iso

$(BUILD):
	@mkdir -p $(BUILD) $(ISO)/boot/grub $(ISO)/boot/chrysalis $(ISO)/system

# Bootloader entry point
$(BUILD)/boot.o: ../os/kernel/boot.asm
	@echo "[BUILD] boot.asm"
	$(AS) $(ASFLAGS) $< -o $@

# Multiboot2 header
$(BUILD)/multiboot2_header.o: ../os/arch/i386/multiboot2_header.c
	@echo "[BUILD] multiboot2_header.c"
	$(CC) $(CFLAGS) -c $< -o $@

# Kernel entry point and installer implementation
$(BUILD)/kernel_installer.o: kernel.cpp
	@echo "[BUILD] kernel.cpp (kernel entry + installer)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Real installer implementation
$(BUILD)/installer.o: installer.cpp
	@echo "[BUILD] installer.cpp (real installation)"
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Stubs for missing kernel symbols
$(BUILD)/stubs.o: stubs.cpp
	@echo "[BUILD] stubs.cpp"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Kernel ELF binary
$(BUILD)/installer.elf: $(OBJS)
	@echo "[LINK] Creating installer.elf"
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# ISO creation
installer.iso: $(BUILD)/installer.elf
	@echo "[ISO] Preparing assets..."
	@cp $(BUILD)/installer.elf $(ISO)/boot/
	@# Copy Assets from Main OS Build
	@if [ -f ../os/iso/boot/kernel.bin ]; then cp ../os/iso/boot/kernel.bin $(ISO)/boot/chrysalis/; else echo "WARNING: kernel.bin not found!"; fi
	@if [ -f ../os/iso/icons.mod ]; then cp ../os/iso/icons.mod $(ISO)/system/; else echo "WARNING: icons.mod not found!"; fi
	@
	@echo "[ISO] Generating GRUB config..."
	@echo 'set timeout=5' > $(ISO)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO)/boot/grub/grub.cfg
	@echo '' >> $(ISO)/boot/grub/grub.cfg
	@echo 'menuentry "Chrysalis OS - Installer" {' >> $(ISO)/boot/grub/grub.cfg
	@echo '  multiboot2 /boot/installer.elf' >> $(ISO)/boot/grub/grub.cfg
	@echo '  module2 /boot/chrysalis/kernel.bin kernel.bin' >> $(ISO)/boot/grub/grub.cfg
	@echo '  module2 /system/icons.mod icons.mod' >> $(ISO)/boot/grub/grub.cfg
	@echo '  boot' >> $(ISO)/boot/grub/grub.cfg
	@echo '}' >> $(ISO)/boot/grub/grub.cfg
	@
	@echo "[ISO] Generating ISO image..."
	@grub-mkrescue -o installer.iso $(ISO) 2>&1 | grep -v "xorriso:"
	@ls -lh installer.iso | awk '{print "[ISO] Created: " $$9 " (" $$5 " bytes)"}'

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD) $(ISO) installer.iso

.PHONY: all clean

