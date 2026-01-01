#include "serial.h"
#include "../arch/i386/io.h"

#define COM1 0x3F8

void serial_init()
{
    outb(COM1 + 1, 0x00);    // Disable interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB
    outb(COM1 + 0, 0x03);    // Baud rate divisor (lo byte) = 3 → 38400
    outb(COM1 + 1, 0x00);    // hi byte
    outb(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int serial_received()
{
    return inb(COM1 + 5) & 1;
}

char serial_read()
{
    while (!serial_received());
    return inb(COM1);
}

int serial_is_transmit_empty()
{
    return inb(COM1 + 5) & 0x20;
}

void serial_write(char c)
{
    while (!serial_is_transmit_empty());
    outb(COM1, c);
}

void serial_write_string(const char* str)
{
    while (*str)
        serial_write(*str++);
}
