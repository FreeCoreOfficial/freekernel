# Chrysalis OS

A custom operating system built from scratch using C++, starting with a terminal interface and designed for future GUI support.

## Project Overview

**Name:** Chrysalis OS (chosen for its metaphor of transformation - from terminal to GUI)

**Language:** C++ with Assembly for boot code

**Bootloader:** GRUB (Grand Unified Bootloader)

**Development Platform:** Linux (Debian-based)

**Current Status:** Initial setup phase

## Project Goals

- Build an OS from scratch using C++
- Start with terminal/CLI interface
- Design with future GUI support in mind
- Learn OS development concepts hands-on

## Development Environment

### Required Packages

The project requires several package groups for development:

#### 1. Core Development Tools (Essential)
- `build-essential` - Essential compilation tools
- `nasm` - Assembler for boot code
- `gcc` - C compiler
- `g++` - C++ compiler
- `make` - Build automation
- `binutils` - Binary utilities (linker, assembler)
- `gdb` - Debugger

#### 2. QEMU Emulator
- `qemu-system-x86` - Virtual machine for testing the OS
- `qemu-utils` - QEMU utilities

#### 3. Bootloader Tools
- `grub-pc-bin` - GRUB bootloader binaries
- `xorriso` - ISO image creation
- `mtools` - FAT filesystem tools

#### 4. Optional Tools
- `git` - Version control
- `cmake` - Advanced build system (if needed later)
- `clang` - Alternative C++ compiler
- `lldb` - LLVM debugger
- `valgrind` - Memory debugging

### Installation

A package manager script (`install.sh`) is provided that:
- Automatically detects your Linux distribution (Debian, Ubuntu, Arch, Fedora, etc.)
- Allows selective installation of package groups
- Provides package removal functionality
- Converts package names automatically for different distros

**Usage:**
```bash
chmod +x install.sh
sudo ./install.sh
```

The script provides an interactive menu to install or remove packages by group.

### Verifying GRUB Installation

Check if GRUB tools are available:
```bash
which grub-mkrescue
```

Should return: `/usr/bin/grub-mkrescue`

If not installed:
```bash
sudo apt install grub-pc-bin xorriso mtools
```

## Architecture Decisions

### Why GRUB?

**GRUB (Grand Unified Bootloader)** was chosen as the bootloader because:
- It's a general-purpose bootloader (not Linux-specific)
- Works with any OS following the Multiboot specification
- Handles complex boot processes (protected mode switching, kernel loading)
- Well-tested and reliable
- Allows focus on kernel development instead of low-level boot code
- Industry standard approach

**How GRUB is used:**
- GRUB is NOT installed on the development machine
- GRUB binaries are bundled into the bootable ISO
- `grub-mkrescue` creates ISO files containing GRUB + our kernel
- The ISO is then booted in QEMU for testing

## Planned Project Structure

```
chrysalis/
├── boot/          # Bootloader configuration
│   └── grub.cfg   # GRUB configuration file
├── kernel/        # Kernel source code
│   ├── kernel.cpp # Main kernel entry point
│   └── boot.asm   # Assembly bootstrap code
├── include/       # Header files
│   └── *.h        # Kernel headers
├── lib/           # Standard library implementations
├── build/         # Compiled object files (generated)
├── iso/           # ISO image creation (generated)
│   └── boot/
│       └── grub/
└── Makefile       # Build automation script
```

## Next Steps

1. **Create project structure** - Set up all necessary directories
2. **Write minimal kernel** - Create a "Hello Chrysalis OS" kernel that boots
3. **Configure GRUB** - Create grub.cfg to load our kernel
4. **Create Makefile** - Automate compilation and ISO creation
5. **Test in QEMU** - Boot the ISO and verify it works

## Development Workflow

Once set up, the typical workflow will be:

1. Write/modify kernel code
2. Run `make` to compile and create bootable ISO
3. Run `make run` to test in QEMU
4. Debug and iterate

## Technical Notes

- **Multiboot Specification:** Our kernel will follow the Multiboot specification so GRUB can load it
- **Cross-compilation:** We're compiling for bare metal (no OS), so special compiler flags are needed
- **Testing:** QEMU provides a safe virtual environment for testing without risking real hardware
- **ISO Format:** The final bootable image will be in ISO 9660 format with El Torito boot specification

## Resources & Documentation

- Multiboot Specification: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
- OSDev Wiki: https://wiki.osdev.org/
- GRUB Manual: https://www.gnu.org/software/grub/manual/

## License

[To be determined]

## Contributing

[To be determined]

---

**Last Updated:** December 31, 2025  
**Project Status:** Initial Setup Phase  
**Current Version:** 0.0.1-pre-alpha