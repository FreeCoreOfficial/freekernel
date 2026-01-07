#include "fb_console.h"
#include "framebuffer.h"
#include "font8x16.h"
#include "../drivers/serial.h"
#include "../string.h"

/* Import serial logging from kernel glue */
extern void serial(const char *fmt, ...);

/* Console State */
static uint32_t cons_width = 0;
static uint32_t cons_height = 0;
static uint32_t cons_pitch = 0;
static uint8_t  cons_bpp = 0;
static uint8_t* cons_buffer = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t max_cols = 0;
static uint32_t max_rows = 0;

#define FONT_W 8
#define FONT_H 16
#define FG_COLOR 0x00FFFFFF /* White */
#define BG_COLOR 0x00000000 /* Black */

/* Helper to draw a character at specific coordinates */
static void draw_char(uint32_t cx, uint32_t cy, char c) {
    if (!cons_buffer) return;

    const uint8_t* glyph = &font8x16[(uint8_t)c * 16];
    uint32_t start_y = cy * FONT_H;
    uint32_t start_x = cx * FONT_W;

    for (int y = 0; y < FONT_H; y++) {
        uint8_t row = glyph[y];
        for (int x = 0; x < FONT_W; x++) {
            uint32_t color = (row & (1 << (7 - x))) ? FG_COLOR : BG_COLOR;
            
            /* Fast pixel put (assuming 32bpp for now based on kernel config) */
            /* For generic support, we should use fb_putpixel or handle bpp */
            uint32_t offset = (start_y + y) * cons_pitch + (start_x + x) * (cons_bpp / 8);
            
            if (cons_bpp == 32) {
                *(uint32_t*)(cons_buffer + offset) = color;
            } else if (cons_bpp == 24) {
                uint8_t* p = cons_buffer + offset;
                p[0] = color & 0xFF;
                p[1] = (color >> 8) & 0xFF;
                p[2] = (color >> 16) & 0xFF;
            }
        }
    }
}

/* Scroll the screen up by one line */
static void scroll(void) {
    if (!cons_buffer) return;

    uint32_t row_bytes = cons_pitch * FONT_H;
    uint32_t screen_bytes = cons_pitch * cons_height;
    uint32_t copy_size = screen_bytes - row_bytes;

    /* Move memory up */
    memcpy(cons_buffer, cons_buffer + row_bytes, copy_size);

    /* Clear last line */
    uint8_t* last_line = cons_buffer + copy_size;
    memset(last_line, 0, row_bytes); // Assuming 0 is black
}

/* Draw cursor (block) */
static void draw_cursor(int on) {
    if (!cons_buffer) return;
    
    uint32_t start_y = cursor_y * FONT_H;
    uint32_t start_x = cursor_x * FONT_W;
    uint32_t color = on ? 0x00AAAAAA : BG_COLOR; // Grey block for cursor

    /* Draw a block at cursor position (last 2 lines of the char cell) */
    for (int y = FONT_H - 2; y < FONT_H; y++) {
        for (int x = 0; x < FONT_W; x++) {
             uint32_t offset = (start_y + y) * cons_pitch + (start_x + x) * (cons_bpp / 8);
             if (cons_bpp == 32) {
                *(uint32_t*)(cons_buffer + offset) = color;
            } else if (cons_bpp == 24) {
                uint8_t* p = cons_buffer + offset;
                p[0] = color & 0xFF;
                p[1] = (color >> 8) & 0xFF;
                p[2] = (color >> 16) & 0xFF;
            }
        }
    }
}

void fb_cons_init(void) {
    serial("[FB_CONS] Initializing...\n");
    
    fb_get_info(&cons_width, &cons_height, &cons_pitch, &cons_bpp, &cons_buffer);
    
    if (!cons_buffer) {
        serial("[FB_CONS] Error: Framebuffer not available.\n");
        return;
    }

    max_cols = cons_width / FONT_W;
    max_rows = cons_height / FONT_H;
    cursor_x = 0;
    cursor_y = 0;

    serial("[FB_CONS] Resolution: %dx%d, Grid: %dx%d\n", cons_width, cons_height, max_cols, max_rows);
    
    /* Clear screen */
    fb_clear(BG_COLOR);
    serial("[FB_CONS] Screen cleared.\n");
    
    draw_cursor(1);
}

void fb_cons_putc(char c) {
    if (!cons_buffer) return;

    /* Hide cursor before drawing */
    draw_cursor(0);

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            draw_char(cursor_x, cursor_y, ' '); // Erase char
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = max_cols - 1;
            draw_char(cursor_x, cursor_y, ' ');
        }
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else if (c >= ' ') {
        draw_char(cursor_x, cursor_y, c);
        cursor_x++;
    }

    /* Wrap text */
    if (cursor_x >= max_cols) {
        cursor_x = 0;
        cursor_y++;
    }

    /* Scroll if needed */
    if (cursor_y >= max_rows) {
        scroll();
        cursor_y = max_rows - 1;
    }

    /* Show cursor */
    draw_cursor(1);
}

void fb_cons_puts(const char* s) {
    while (*s) {
        fb_cons_putc(*s++);
    }
}