// kernel/panic.cpp

#include "panic.h"
#include "arch/i386/io.h"
#include "panic_sys.h"
#include "video/font8x16.h"
#include "video/gpu.h"
// === Freestanding headers (fără libc) ===
typedef unsigned int size_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

// === Fallback strlen sigur ===
static size_t strlen(const char *str) {
  const char *s = str;
  while (*s)
    ++s;
  return s - str;
}

// === VGA Text Mode ===
#define VGA_MEM ((volatile uint16_t *)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// === Chrysalis Panic Palette  ===
#define CHR_BG 0      // Black
#define CHR_FG 15     // White
#define CHR_ACC_BG 3  // Dark cyan
#define CHR_ACC_FG 15 // White
#define CHR_ATTR ((CHR_BG << 4) | (CHR_FG & 0x0F))
#define CHR_ACC_ATTR ((CHR_ACC_BG << 4) | (CHR_ACC_FG & 0x0F))

// === Framebuffer Colors (ARGB) ===
#define FB_BG 0x0012171D   // deep slate
#define FB_TEXT 0x00F0F3F7 // near white
#define FB_ACC 0x0028C7B7  // teal accent

// === Serial debug (COM1) ===
static void serial_print(const char *s) {
  while (*s) {
    while (!(inb(0x3F8 + 5) & 0x20))
      ;
    outb(0x3F8, *s++);
  }
}

// === Framebuffer Helpers ===
static bool use_fb = false;
static uint32_t fb_w = 0, fb_h = 0;

/* Use GPU subsystem instead of raw framebuffer */
static void check_fb() {
  gpu_device_t *gpu = gpu_get_primary();
  if (gpu && gpu->virt_addr) {
    use_fb = true;
    fb_w = gpu->width;
    fb_h = gpu->height;
  } else {
    use_fb = false;
  }
}

static uint32_t fb_text_color = FB_TEXT;
static uint8_t vga_text_attr = CHR_ATTR;

static void fb_draw_char_color(int x, int y, char c, uint32_t color) {
  gpu_device_t *gpu = gpu_get_primary();
  if (!gpu)
    return;

  const uint8_t *glyph = &font8x16[(uint8_t)c * 16];
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++) {
      if (glyph[i] & (1 << (7 - j))) {
        gpu->ops->putpixel(gpu, x + j, y + i, color);
      }
    }
  }
}

// === Primitivă de scriere caracter ===
static void put_char(int x, int y, char c) {
  if (use_fb) {
    // Pe FB folosim coordonate pixel (8x16 font)
    fb_draw_char_color(x * 8, y * 16, c, fb_text_color);
  } else {
    // Pe VGA folosim coordonate text
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
      VGA_MEM[y * VGA_WIDTH + x] = ((uint16_t)vga_text_attr << 8) | (uint8_t)c;
  }
}

// === Scrie string centrat sau la poziție fixă ===
static void draw_string(int x, int y, const char *s) {
  while (*s) {
    put_char(x++, y, *s++);
  }
}

static void set_text_color(uint32_t fb_color, uint8_t vga_attr) {
  fb_text_color = fb_color;
  vga_text_attr = vga_attr;
}

static void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
  gpu_device_t *gpu = gpu_get_primary();
  if (!gpu)
    return;
  if (gpu->ops && gpu->ops->fillrect) {
    gpu->ops->fillrect(gpu, x, y, w, h, color);
    return;
  }
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      gpu->ops->putpixel(gpu, x + xx, y + yy, color);
    }
  }
}

static void draw_string_center(int y, const char *s) __attribute__((unused));
static void draw_string_center(int y, const char *s) {
  size_t len = strlen(s);
  int x = (VGA_WIDTH - (int)len) / 2;
  draw_string(x, y, s);
}

// === Umple tot ecranul cu fundal albastru + spații (autentic XP) ===
static void clear_screen_chrysalis() {
  if (use_fb) {
    gpu_device_t *gpu = gpu_get_primary();
    if (gpu)
      gpu->ops->clear(gpu, FB_BG);
  } else {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
      VGA_MEM[i] = ((uint16_t)CHR_ATTR << 8) | ' ';
    }
  }
}

// === Convert u32 → string (fără diviziune libc) ===
static void u32_to_hex(uint32_t value, char *buf) {
  const char *hex = "0123456789ABCDEF";
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 9; i >= 2; i--) {
    buf[i] = hex[value & 0xF];
    value >>= 4;
  }
  buf[10] = 0;
}

static void u32_to_dec(uint32_t value, char *buf) __attribute__((unused));
static void u32_to_dec(uint32_t value, char *buf) {
  char temp[12];
  int i = 0;
  if (value == 0) {
    buf[0] = '0';
    buf[1] = 0;
    return;
  }
  while (value) {
    temp[i++] = (value % 10) + '0';
    value /= 10;
  }
  int j = 0;
  while (i > 0)
    buf[j++] = temp[--i];
  buf[j] = 0;
}

extern "C" void panic_render_pretty(const char *msg) {
  check_fb(); /* Detect video mode */
  clear_screen_chrysalis();

  if (!use_fb) {
    /* Legacy VGA fallback */
    draw_string(2, 2, "!! KERNEL PANIC !!");
    draw_string(2, 4, msg ? msg : "Fatal error");
    return;
  }

  /* Centered Modal Design */
  int modal_w = 600;
  int modal_h = 400;
  int mx = (fb_w - modal_w) / 2;
  int my = (fb_h - modal_h) / 2;

  /* Shadow */
  fb_fill_rect(mx + 8, my + 8, modal_w, modal_h, 0xFF000000);
  /* Border & Background */
  fb_fill_rect(mx, my, modal_w, modal_h, FB_ACC);
  fb_fill_rect(mx + 2, my + 2, modal_w - 4, modal_h - 4, FB_BG);

  /* Title Bar */
  fb_fill_rect(mx + 2, my + 2, modal_w - 4, 30, FB_ACC);
  set_text_color(0xFFFFFFFF, CHR_FG);
  draw_string(mx / 8 + 2, my / 16 + 1, "SYSTEM HALT - Chrysalis OS");

  int line_y = my / 16 + 4;
  int line_x = mx / 8 + 4;

  set_text_color(FB_TEXT, CHR_FG);
  draw_string(line_x, line_y++,
              "A fatal error has occurred and the kernel must stop.");
  line_y++;

  set_text_color(FB_ACC, CHR_FG);
  draw_string(line_x, line_y++, "REASON:");
  set_text_color(FB_TEXT, CHR_FG);

  if (msg && msg[0]) {
    char buffer[70];
    const char *p = msg;
    while (*p) {
      int i = 0;
      while (*p && *p != '\n' && i < 68) {
        buffer[i++] = *p++;
      }
      buffer[i] = 0;
      draw_string(line_x + 2, line_y++, buffer);
      if (*p == '\n')
        p++;
      if (line_y > (my + modal_h) / 16 - 8)
        break;
    }
  } else {
    draw_string(line_x + 2, line_y++, "Unknown fatal exception.");
  }

  line_y = (my + modal_h) / 16 - 10;
  set_text_color(FB_ACC, CHR_ACC_FG);
  draw_string(line_x, line_y++, "HARDWARE INFO:");
  set_text_color(FB_TEXT, CHR_FG);

  const char *cpu = panic_sys_cpu_str();
  draw_string(line_x + 2, line_y, "CPU: ");
  draw_string(line_x + 7, line_y++, cpu ? cpu : "unknown");

  char numbuf[12];
  draw_string(line_x + 2, line_y, "RAM: ");
  u32_to_dec(panic_sys_total_ram_kb() / 1024, numbuf);
  draw_string(line_x + 7, line_y, numbuf);
  draw_string(line_x + 7 + (int)strlen(numbuf), line_y++, " MB");

  line_y = (my + modal_h) / 16 - 4;
  fb_fill_rect(mx + 20, (my + modal_h) - 60, modal_w - 40, 40, 0xFF1D2833);
  set_text_color(0x0028C7B7, CHR_FG);
  draw_string(line_x + 2, line_y, "Press [R] to Restart | [H] to Shutdown");
}

// === Reboot forțat ===
extern "C" void try_reboot() {
  // 1. 8042 Keyboard Controller Pulse
  outb(0x64, 0xFE);
  for (volatile int i = 0; i < 100000; i++)
    ;

  // 2. Fast A20/Reset via System Control Port A (0x92)
  uint8_t val = inb(0x92);
  outb(0x92, val | 1);
  for (volatile int i = 0; i < 100000; i++)
    ;

  // 3. Fallback: Triple Fault
  struct {
    uint16_t limit;
    uint32_t base;
  } __attribute__((packed)) idt = {0, 0};
  asm volatile("lidt %0; int $3" : : "m"(idt));
}

// === Handler panic principal ===
extern "C" void panic_ex(const char *msg, uint32_t eip, uint32_t cs,
                         uint32_t eflags) {
  (void)eip;
  (void)cs;
  (void)eflags;
  asm volatile("cli"); // dezactivează interrupțiile

  serial_print("\n!!! KERNEL PANIC !!!\n");
  if (msg) {
    serial_print(msg);
  }
  serial_print("\nEIP: ");
  char hexbuf[12];
  u32_to_hex(eip, hexbuf);
  serial_print(hexbuf);
  serial_print("\n");

  panic_render_pretty(msg);

  /* Loop infinit, așteaptă tasta R pentru reboot sau H pentru halt (loop) */
  for (;;) {
    if (inb(0x64) & 1) { // key ready
      uint8_t scancode = inb(0x60);
      if (scancode == 0x13 || scancode == 0x32) { // R or M
        try_reboot();
      }
      if (scancode == 0x23) { // H (Halt)
        /* We are already halting in a loop, so just stay here.
           In a real system, we might trigger an ACPI shutdown. */
      }
    }
  }
}

extern "C" void panic(const char *msg) { panic_ex(msg, 0, 0, 0); }
