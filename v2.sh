#!/bin/bash

# Chrysalis OS - Interactive Terminal Update Script
# Adds keyboard input, shell, and commands

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ ! -d "chrysalis" ]; then
    echo -e "${YELLOW}Error: chrysalis directory not found!${NC}"
    echo "Please run the setup script first."
    exit 1
fi

cd chrysalis

echo -e "${BLUE}Adding interactive terminal features to Chrysalis OS...${NC}"

# Create keyboard driver header
echo -e "${GREEN}Creating keyboard driver...${NC}"
cat > include/keyboard.h << 'EOF'
// Chrysalis OS - Keyboard Driver Header

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_initialize();
char keyboard_getchar();
bool keyboard_has_input();

#endif
EOF

# Create keyboard driver implementation
cat > kernel/keyboard.cpp << 'EOF'
// Chrysalis OS - Keyboard Driver Implementation

#include "../include/keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// US QWERTY keyboard layout
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void keyboard_initialize() {
    // Keyboard is already initialized by BIOS
    // Just clear the buffer
    while (inb(KEYBOARD_STATUS_PORT) & 1) {
        inb(KEYBOARD_DATA_PORT);
    }
}

bool keyboard_has_input() {
    return (inb(KEYBOARD_STATUS_PORT) & 1) != 0;
}

char keyboard_getchar() {
    while (!keyboard_has_input()) {
        asm volatile("hlt");
    }
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Only handle key press events (bit 7 = 0)
    if (scancode & 0x80) {
        return 0;
    }
    
    if (scancode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[scancode];
    }
    
    return 0;
}
EOF

# Create string utilities header
echo -e "${GREEN}Creating string utilities...${NC}"
cat > include/string.h << 'EOF'
// Chrysalis OS - String Utilities

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);

#endif
EOF

# Create string utilities implementation
cat > kernel/string.cpp << 'EOF'
// Chrysalis OS - String Utilities Implementation

#include "../include/string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}
EOF

# Create commands header
echo -e "${GREEN}Creating command system...${NC}"
cat > include/commands.h << 'EOF'
// Chrysalis OS - Command System Header

#ifndef COMMANDS_H
#define COMMANDS_H

void cmd_clear();
void cmd_echo(const char* args);
void cmd_kernel_version();
void cmd_shutdown();
void cmd_help();

#endif
EOF

# Create commands implementation
cat > kernel/commands.cpp << 'EOF'
// Chrysalis OS - Command Implementations

#include "../include/commands.h"
#include "../include/terminal.h"

void cmd_clear() {
    terminal_clear();
}

void cmd_echo(const char* args) {
    if (args && args[0] != '\0') {
        terminal_writestring(args);
    }
    terminal_putchar('\n');
}

void cmd_kernel_version() {
    terminal_writestring("Chrysalis OS version 0.1.0\n");
    terminal_writestring("Build: Interactive Terminal Mode\n");
    terminal_writestring("Architecture: x86 (32-bit)\n");
}

void cmd_shutdown() {
    terminal_writestring("Shutting down Chrysalis OS...\n");
    terminal_writestring("Goodbye!\n");
    
    // QEMU shutdown
    asm volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
    
    // If QEMU shutdown fails, halt
    while(1) {
        asm volatile("hlt");
    }
}

void cmd_help() {
    terminal_writestring("Chrysalis OS - Available Commands:\n\n");
    terminal_writestring("  clear          - Clear the screen\n");
    terminal_writestring("  echo [text]    - Print text to screen\n");
    terminal_writestring("  kernel-version - Display kernel version\n");
    terminal_writestring("  shutdown       - Shutdown the system\n");
    terminal_writestring("  help           - Show this help message\n");
    terminal_writestring("\n");
}
EOF

# Create shell header
echo -e "${GREEN}Creating shell interface...${NC}"
cat > include/shell.h << 'EOF'
// Chrysalis OS - Shell Header

#ifndef SHELL_H
#define SHELL_H

void shell_initialize();
void shell_run();

#endif
EOF

# Create shell implementation
cat > kernel/shell.cpp << 'EOF'
// Chrysalis OS - Shell Implementation

#include "../include/shell.h"
#include "../include/terminal.h"
#include "../include/keyboard.h"
#include "../include/string.h"
#include "../include/commands.h"

#define MAX_COMMAND_LENGTH 256

static char command_buffer[MAX_COMMAND_LENGTH];
static size_t command_pos = 0;

static void print_prompt() {
    terminal_writestring("chrysalis> ");
}

static void parse_and_execute(const char* input) {
    // Skip leading spaces
    while (*input == ' ') input++;
    
    // Empty command
    if (*input == '\0') {
        return;
    }
    
    // Find command end (space or null)
    size_t cmd_len = 0;
    while (input[cmd_len] && input[cmd_len] != ' ') {
        cmd_len++;
    }
    
    // Find arguments (after first space)
    const char* args = input + cmd_len;
    while (*args == ' ') args++;
    
    // Execute commands
    if (strncmp(input, "clear", 5) == 0 && cmd_len == 5) {
        cmd_clear();
    }
    else if (strncmp(input, "echo", 4) == 0 && cmd_len == 4) {
        cmd_echo(args);
    }
    else if (strncmp(input, "kernel-version", 14) == 0 && cmd_len == 14) {
        cmd_kernel_version();
    }
    else if (strncmp(input, "shutdown", 8) == 0 && cmd_len == 8) {
        cmd_shutdown();
    }
    else if (strncmp(input, "help", 4) == 0 && cmd_len == 4) {
        cmd_help();
    }
    else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(input);
        terminal_writestring("\nType 'help' for available commands.\n");
    }
}

void shell_initialize() {
    command_pos = 0;
    for (size_t i = 0; i < MAX_COMMAND_LENGTH; i++) {
        command_buffer[i] = '\0';
    }
}

void shell_run() {
    terminal_writestring("Welcome to Chrysalis OS!\n");
    terminal_writestring("Type 'help' for available commands.\n\n");
    
    print_prompt();
    
    while (1) {
        char c = keyboard_getchar();
        
        if (c == 0) {
            continue;
        }
        
        if (c == '\n') {
            // Execute command
            terminal_putchar('\n');
            command_buffer[command_pos] = '\0';
            parse_and_execute(command_buffer);
            
            // Reset for next command
            command_pos = 0;
            command_buffer[0] = '\0';
            print_prompt();
        }
        else if (c == '\b') {
            // Backspace
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                
                // Move cursor back and clear character
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            }
        }
        else if (command_pos < MAX_COMMAND_LENGTH - 1) {
            // Regular character
            command_buffer[command_pos++] = c;
            terminal_putchar(c);
        }
    }
}
EOF

# Update kernel main
echo -e "${GREEN}Updating kernel main...${NC}"
cat > kernel/kernel.cpp << 'EOF'
// Chrysalis OS - Kernel Entry Point

#include "../include/terminal.h"
#include "../include/keyboard.h"
#include "../include/shell.h"

extern "C" void kernel_main() {
    // Initialize subsystems
    terminal_initialize();
    terminal_clear();
    keyboard_initialize();
    shell_initialize();
    
    // Start interactive shell
    shell_run();
    
    // Should never reach here
    while(1) {
        asm volatile("hlt");
    }
}
EOF

# Update Makefile
echo -e "${GREEN}Updating Makefile...${NC}"
cat > Makefile << 'EOF'
# Chrysalis OS - Makefile

AS = nasm
CC = gcc
CXX = g++
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti
CXXFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -fno-use-cxa-atexit
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Source files
BOOT_SRC = boot/boot.asm
KERNEL_SRC = kernel/kernel.cpp kernel/terminal.cpp kernel/keyboard.cpp \
             kernel/string.cpp kernel/shell.cpp kernel/commands.cpp

# Object files
BOOT_OBJ = build/boot.o
KERNEL_OBJ = build/kernel.o build/terminal.o build/keyboard.o \
             build/string.o build/shell.o build/commands.o

# Output
KERNEL_BIN = build/chrysalis.bin
ISO_FILE = chrysalis.iso

.PHONY: all clean run iso

all: iso

# Assemble boot code
build/boot.o: boot/boot.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

# Compile kernel files
build/kernel.o: kernel/kernel.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/terminal.o: kernel/terminal.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/keyboard.o: kernel/keyboard.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/string.o: kernel/string.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/shell.o: kernel/shell.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/commands.o: kernel/commands.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link kernel
$(KERNEL_BIN): $(BOOT_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

# Create bootable ISO
iso: $(KERNEL_BIN)
	@mkdir -p iso/boot
	@cp $(KERNEL_BIN) iso/boot/chrysalis.bin
	grub-mkrescue -o $(ISO_FILE) iso
	@echo "ISO created: $(ISO_FILE)"

# Run in QEMU
run: iso
	qemu-system-i386 -cdrom $(ISO_FILE)

# Clean build files
clean:
	rm -rf build/*.o build/*.bin $(ISO_FILE)
	rm -rf iso/boot/chrysalis.bin

# Full clean
distclean: clean
	rm -rf build iso/boot/chrysalis.bin
EOF

echo ""
echo -e "${GREEN}✓ Interactive terminal features added successfully!${NC}"
echo ""
echo -e "${YELLOW}New features:${NC}"
echo "  • Keyboard input driver"
echo "  • Interactive shell with command prompt"
echo "  • Command parser"
echo "  • Commands: clear, echo, kernel-version, shutdown, help"
echo "  • Backspace support"
echo ""
echo -e "${BLUE}To build and run:${NC}"
echo "  make clean"
echo "  make run"
echo ""
echo -e "${GREEN}Try typing commands in the shell!${NC}"