/* Stubs for Installer to satisfy linker dependencies */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

extern "C" {

/* Basic VGA Text Mode Driver */
static uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;
static int terminal_row = 0;
static int terminal_column = 0;
static uint8_t terminal_color = 0x07; // Light Grey on Black

void terminal_putentryat(char c, uint8_t color, int x, int y) {
  const int index = y * VGA_WIDTH + x;
  VGA_MEMORY[index] = (uint16_t)c | (uint16_t)color << 8;
}

void terminal_scroll() {
  for (int y = 0; y < VGA_HEIGHT - 1; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      const int src_index = (y + 1) * VGA_WIDTH + x;
      const int dst_index = y * VGA_WIDTH + x;
      VGA_MEMORY[dst_index] = VGA_MEMORY[src_index];
    }
  }
  for (int x = 0; x < VGA_WIDTH; x++) {
    const int index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
    VGA_MEMORY[index] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
  }
}

void terminal_putchar(char c) {
  if (c == '\n') {
    terminal_column = 0;
    if (++terminal_row == VGA_HEIGHT) {
      terminal_row = VGA_HEIGHT - 1;
      terminal_scroll();
    }
  } else {
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
      terminal_column = 0;
      if (++terminal_row == VGA_HEIGHT) {
        terminal_row = VGA_HEIGHT - 1;
        terminal_scroll();
      }
    }
  }
}

void terminal_writestring(const char *s) {
  while (*s)
    terminal_putchar(*s++);
}

void terminal_set_color(uint8_t c) { terminal_color = c; }

/* Also redirect generic printf to VGA + Serial */
void terminal_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  // Print to serial (if needed, but serial() does that)
  // We duplicate logic or just use a helper buffer?
  // Simplified printf logic for VGA:

  // Note: We can reuse the logic from serial() if we factor it out,
  // or just implement a minimal one here directly.

  while (*fmt) {
    if (*fmt != '%') {
      terminal_putchar(*fmt++);
      continue;
    }
    fmt++;
    switch (*fmt) {
    case 's':
      terminal_writestring(va_arg(args, const char *));
      break;
    case 'c':
      terminal_putchar((char)va_arg(args, int));
      break;
    case 'd': {
      int v = va_arg(args, int);
      if (v < 0) {
        terminal_putchar('-');
        v = -v;
      }
      if (v == 0)
        terminal_putchar('0');
      else {
        char buf[12];
        int i = 0;
        while (v) {
          buf[i++] = '0' + (v % 10);
          v /= 10;
        }
        while (i--)
          terminal_putchar(buf[i]);
      }
    } break;
    case 'x': {
      uint32_t v = va_arg(args, uint32_t);
      terminal_writestring("0x");
      for (int i = 28; i >= 0; i -= 4) {
        int d = (v >> i) & 0xF;
        terminal_putchar(d < 10 ? '0' + d : 'a' + d - 10);
      }
    } break;
    }
    fmt++;
  }

  va_end(args);
}

/* Wrapper for serial() expected by other modules - C Linkage */
extern "C" {

extern void serial_write(char c);
extern void serial_write_string(const char *s);

void serial(const char *fmt, ...) {
  if (!fmt)
    return;

  va_list args;
  va_start(args, fmt);

  // 1. Output to Serial
  va_list args_serial;
  va_copy(args_serial, args);

  const char *f1 = fmt;
  while (*f1) {
    if (*f1 != '%') {
      serial_write(*f1++);
      continue;
    }
    f1++;
    if (!*f1)
      break;
    switch (*f1) {
    case 's': {
      const char *s = va_arg(args_serial, const char *);
      if (s)
        serial_write_string(s);
      else
        serial_write_string("(null)");
    } break;
    case 'c':
      serial_write((char)va_arg(args_serial, int));
      break;
    case 'd':
    case 'u': {
      int v = va_arg(args_serial, int);
      if (*f1 == 'd' && v < 0) {
        serial_write('-');
        v = -v;
      }
      if (v == 0)
        serial_write('0');
      else {
        char buf[12];
        int i = 0;
        uint32_t val = (uint32_t)v;
        while (val) {
          buf[i++] = '0' + (val % 10);
          val /= 10;
        }
        while (i--)
          serial_write(buf[i]);
      }
    } break;
    case 'p':
    case 'x': {
      uint32_t v = va_arg(args_serial, uint32_t);
      serial_write('0');
      serial_write('x');
      for (int i = 28; i >= 0; i -= 4) {
        int d = (v >> i) & 0xF;
        serial_write(d < 10 ? '0' + d : 'a' + d - 10);
      }
    } break;
    case '%':
      serial_write('%');
      break;
    }
    f1++;
  }
  va_end(args_serial);

  // 2. Output to VGA
  va_list args_vga;
  va_copy(args_vga, args);
  const char *f2 = fmt;
  while (*f2) {
    if (*f2 != '%') {
      terminal_putchar(*f2++);
      continue;
    }
    f2++;
    if (!*f2)
      break;
    switch (*f2) {
    case 's': {
      const char *s = va_arg(args_vga, const char *);
      if (s)
        terminal_writestring(s);
      else
        terminal_writestring("(null)");
    } break;
    case 'c':
      terminal_putchar((char)va_arg(args_vga, int));
      break;
    case 'd':
    case 'u': {
      int v = va_arg(args_vga, int);
      if (*f2 == 'd' && v < 0) {
        terminal_putchar('-');
        v = -v;
      }
      if (v == 0)
        terminal_putchar('0');
      else {
        char buf[12];
        int i = 0;
        uint32_t val = (uint32_t)v;
        while (val) {
          buf[i++] = '0' + (val % 10);
          val /= 10;
        }
        while (i--)
          terminal_putchar(buf[i]);
      }
    } break;
    case 'p':
    case 'x': {
      uint32_t v = va_arg(args_vga, uint32_t);
      terminal_writestring("0x");
      for (int i = 28; i >= 0; i -= 4) {
        int d = (v >> i) & 0xF;
        terminal_putchar(d < 10 ? '0' + d : 'a' + d - 10);
      }
    } break;
    case '%':
      terminal_putchar('%');
      break;
    }
    f2++;
  }
  va_end(args_vga);

  va_end(args);
}

} /* End extern "C" */

/* Block Cache Stubs - We don't use cache in installer */
/* Assuming block_get returns a buffer or something. disk_read_sector calls it?
 */
/* Wait, disk.cpp uses block_get. We need to check its signature.
   Likely: void* block_get(int device, uint32_t sector);
*/
void *block_get(int device, uint32_t sector) {
  (void)device;
  (void)sector;
  return NULL;
}

/* ATA Stubs if needed */
// void ata_set_allow_mbr_write(int allow) { (void)allow; }

/* Misc */
void panic(const char *msg) {
  // serial("PANIC: %s\n", msg);
  while (1)
    asm volatile("hlt");
}
}
