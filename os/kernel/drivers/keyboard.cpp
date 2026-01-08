// kernel/drivers/keyboard.cpp
#include <stdint.h>
#include <stdbool.h>
#include "keyboard.h"
#include "../interrupts/irq.h"
#include "../arch/i386/io.h"
#include "../drivers/serial.h"
#include "../input/input.h"
#include "../video/fb_console.h"
#include "../interrupts/irq.h"
#include "../hardware/lapic.h"

/* PS/2 Ports */
#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_CMD     0x64

/* US QWERTY Keymap (embedded for stability) */
static const char keymap_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char keymap_us_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* state flags */
static bool ctrl_pressed = false;
static bool shift_pressed = false;
static bool alt_pressed = false;

/* Helpers pentru sincronizare PS/2 */
static void kbd_wait_write() {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS) & 2) == 0) return;
        asm volatile("pause");
    }
    serial_write_string("[PS/2] Warning: Write timeout\r\n");
}

static void kbd_wait_read() {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS) & 1) == 1) return;
        asm volatile("pause");
    }
    // Nu e neapărat eroare, poate nu sunt date
}

extern "C" void keyboard_handler(registers_t* regs)
{
    (void)regs;
    uint8_t status = inb(PS2_STATUS);
    
    /* DEBUG: Log status to see if IRQ fires */
    serial_printf("[KBD] IRQ Status: 0x%x\n", status);

    if (status & 0x01) {

        if (input_is_usb_keyboard_active()) {
            serial_printf("[KBD] USB active, ignoring PS/2\n");
            inb(PS2_DATA);
            goto done;
        }

        if (status & 0x20) {
            uint8_t mdata = inb(PS2_DATA);
            serial_printf("[KBD] Mouse data (0x%x), ignoring\n", mdata);
            goto done;
        }

        uint8_t scancode = inb(PS2_DATA);
        serial_printf("[KBD] Scancode: 0x%x\n", scancode);

        static bool e0_prefix = false;
        if (scancode == 0xE0) {
            e0_prefix = true;
            goto done;
        }

        if (e0_prefix) {
            e0_prefix = false;
            if (scancode == 0x49) fb_cons_scroll(-10);
            else if (scancode == 0x51) fb_cons_scroll(10);
            goto done;
        }

        if (scancode & 0x80) {
            uint8_t sc = scancode & 0x7F;
            if (sc == 0x2A || sc == 0x36) shift_pressed = false;
            else if (sc == 0x1D) ctrl_pressed = false;
            else if (sc == 0x38) alt_pressed = false;
        } else {
            if (scancode == 0x2A || scancode == 0x36) shift_pressed = true;
            else if (scancode == 0x1D) ctrl_pressed = true;
            else if (scancode == 0x38) alt_pressed = true;
            else {
                char c = 0;
                if (scancode < 128)
                    c = shift_pressed ? keymap_us_shift[scancode]
                                      : keymap_us[scancode];

                if (c) {
                    serial_printf("[KBD] Pushing char: %c\n", c);
                    input_push_key((uint32_t)c, true);
                }
            }
        }
    } else {
        serial_printf("[KBD] Spurious IRQ (Status 0x%x)\n", status);
    }

done:
    return;
}


extern "C" void keyboard_init()
{
    serial_write_string("[PS/2] Initializing keyboard driver...\r\n");

    /* 1. Disable Keyboard Port */
    kbd_wait_write();
    outb(PS2_CMD, 0xAD);

    /* 2. Flush Output Buffer */
    while(inb(PS2_STATUS) & 1) inb(PS2_DATA);

    /* 3. Enable Keyboard Port */
    kbd_wait_write();
    outb(PS2_CMD, 0xAE);

    /* 4. Enable Interrupts in Command Byte (Before Reset) */
    kbd_wait_write();
    outb(PS2_CMD, 0x20); // Read Config
    
    kbd_wait_read();
    uint8_t cmd = inb(PS2_DATA);
    
    cmd |= 0x01; // Enable IRQ1 (Keyboard)
    cmd &= ~0x10; // Disable IRQ12 (Mouse) temporarily
    
    kbd_wait_write();
    outb(PS2_CMD, 0x60); // Write Config
    kbd_wait_write();
    outb(PS2_DATA, cmd);

    /* Reset keyboard */
    kbd_wait_write();
    outb(PS2_DATA, 0xFF);
    
    /* Wait for ACK (0xFA) */
    kbd_wait_read();
    if (inb(PS2_STATUS) & 1) {
        uint8_t resp = inb(PS2_DATA);
        serial_printf("[PS/2] Reset ACK: 0x%x\n", resp);
    }

    /* Wait for BAT (0xAA) - Self Test Passed */
    int timeout = 1000000; 
    while(timeout-- > 0) {
        if (inb(PS2_STATUS) & 1) {
            uint8_t bat = inb(PS2_DATA);
            if (bat == 0xAA) {
                serial_write_string("[PS/2] Self Test Passed (0xAA)\r\n");
                break; 
            }
            serial_printf("[PS/2] Reset response: 0x%x\n", bat);
        }
        asm volatile("pause");
    }

    /* Enable scanning */
    kbd_wait_write();
    outb(PS2_DATA, 0xF4);
    
    kbd_wait_read();
    if (inb(PS2_STATUS) & 1) {
        uint8_t resp = inb(PS2_DATA); // ACK for Enable Scanning
        serial_printf("[PS/2] Enable Scanning ACK: 0x%x\n", resp);
    }

    /* 7. Final Flush to ensure line is low before unmasking PIC */
    while(inb(PS2_STATUS) & 1) inb(PS2_DATA);

    irq_install_handler(1, keyboard_handler);
    
    /* 5. Unmask IRQ1 in PIC (Master) explicitly */
    uint8_t mask = inb(0x21);
    outb(0x21, mask & 0xFD); // Clear bit 1 (IRQ1)

    serial_write_string("[PS/2] Keyboard handler installed on IRQ1.\r\n");
    input_signal_ready();
    serial_write_string("[PS/2] Init sequence complete.\r\n");
}
