#include "fb_console.h"
#include "framebuffer.h"
#include "gpu.h"
#include "font8x16.h"
#include "../drivers/serial.h"
#include "../string.h"
#include "../mem/kmalloc.h"

/* Import serial logging from kernel glue */
extern void serial(const char *fmt, ...);

/* Console State */
static uint32_t cons_width = 0;
static uint32_t cons_height = 0;
static uint32_t cons_pitch = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t max_cols = 0;
static uint32_t max_rows = 0;

/* Text Buffer for fast scrolling and redraw (Shadow Buffer) */
typedef struct {
    char c;
    uint32_t fg;
    uint32_t bg;
} console_cell_t;

static console_cell_t* text_buffer = 0;

#define FONT_W 8
#define FONT_H 16
#define DEFAULT_FG 0x00CCCCCC /* Light Grey (Linux-like) */
#define DEFAULT_BG 0x00000000 /* Black */

static uint32_t current_fg = DEFAULT_FG;
static uint32_t current_bg = DEFAULT_BG;

/* Helper to draw a character using GPU ops */
static void draw_char_at(uint32_t cx, uint32_t cy, char c, uint32_t fg, uint32_t bg) {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;

    const uint8_t* glyph = &font8x16[(uint8_t)c * 16];
    uint32_t start_y = cy * FONT_H;
    uint32_t start_x = cx * FONT_W;

    /* Bounds check */
    if (start_x + FONT_W > gpu->width || start_y + FONT_H > gpu->height) return;

    for (int y = 0; y < FONT_H; y++) {
        uint8_t row = glyph[y];
        for (int x = 0; x < FONT_W; x++) {
            uint32_t color = (row & (1 << (7 - x))) ? fg : bg;
            gpu->ops->putpixel(gpu, start_x + x, start_y + y, color);
        }
    }
}

/* Redraw the entire screen from the text buffer */
static void redraw_screen(void) {
    if (!text_buffer) return;
    
    for (uint32_t y = 0; y < max_rows; y++) {
        for (uint32_t x = 0; x < max_cols; x++) {
            console_cell_t* cell = &text_buffer[y * max_cols + x];
            draw_char_at(x, y, cell->c, cell->fg, cell->bg);
        }
    }
}

/* Scroll the screen up by one line using the text buffer */
static void scroll(void) {
    if (!text_buffer) return;

    /* Move text buffer up in RAM (Fast) */
    uint32_t count = (max_rows - 1) * max_cols;
    memmove(text_buffer, text_buffer + max_cols, count * sizeof(console_cell_t));

    /* Clear last row in buffer */
    console_cell_t* last_row = text_buffer + count;
    for (uint32_t i = 0; i < max_cols; i++) {
        last_row[i].c = ' ';
        last_row[i].fg = current_fg;
        last_row[i].bg = current_bg;
    }

    /* Redraw screen from buffer (Faster and cleaner than VRAM read/write) */
    redraw_screen();
}

/* Draw cursor (block) */
static void draw_cursor(int on) {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;
    
    uint32_t start_y = cursor_y * FONT_H;
    uint32_t start_x = cursor_x * FONT_W;
    
    /* Bounds check */
    if (start_x + FONT_W > gpu->width || start_y + FONT_H > gpu->height) return;

    /* If turning off, restore character from buffer */
    if (!on) {
        if (text_buffer) {
            console_cell_t* cell = &text_buffer[cursor_y * max_cols + cursor_x];
            draw_char_at(cursor_x, cursor_y, cell->c, cell->fg, cell->bg);
        } else {
             draw_char_at(cursor_x, cursor_y, ' ', current_fg, current_bg);
        }
        return;
    }

    /* Draw cursor block */
    uint32_t color = 0x00AAAAAA; // Grey block
    for (int y = FONT_H - 2; y < FONT_H; y++) {
        for (int x = 0; x < FONT_W; x++) {
             gpu->ops->putpixel(gpu, start_x + x, start_y + y, color);
        }
    }
}

void fb_cons_init(void) {
    serial("[FB_CONS] Initializing...\n");
    gpu_device_t* gpu = gpu_get_primary();
    
    if (!gpu) {
        serial("[FB_CONS] Error: No primary GPU available.\n");
        return;
    }

    cons_width = gpu->width;
    cons_height = gpu->height;
    cons_pitch = gpu->pitch;

    max_cols = cons_width / FONT_W;
    max_rows = cons_height / FONT_H;
    cursor_x = 0;
    cursor_y = 0;
    
    /* Allocate text buffer */
    if (text_buffer) kfree(text_buffer);
    text_buffer = (console_cell_t*)kmalloc(max_cols * max_rows * sizeof(console_cell_t));
    
    if (!text_buffer) {
        serial("[FB_CONS] Error: Failed to allocate text buffer!\n");
        return;
    }

    /* Initialize text buffer */
    for (uint32_t i = 0; i < max_cols * max_rows; i++) {
        text_buffer[i].c = ' ';
        text_buffer[i].fg = current_fg;
        text_buffer[i].bg = current_bg;
    }

    /* Clear screen */
    gpu->ops->clear(gpu, current_bg);
    serial("[FB_CONS] Screen cleared.\n");
    
    draw_cursor(1);
}

/* Internal putc without cursor handling (for bulk updates) */
static void fb_cons_putc_internal(char c) {

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = max_cols - 1;
        }
        /* Erase char in buffer and screen */
        console_cell_t* cell = &text_buffer[cursor_y * max_cols + cursor_x];
        cell->c = ' ';
        cell->fg = current_fg;
        cell->bg = current_bg;
        draw_char_at(cursor_x, cursor_y, ' ', current_fg, current_bg);

    } else if (c == '\t') {
        int spaces = 8 - (cursor_x % 8);
        for (int i = 0; i < spaces; i++) {
            if (cursor_x < max_cols) {
                console_cell_t* cell = &text_buffer[cursor_y * max_cols + cursor_x];
                cell->c = ' ';
                cell->fg = current_fg;
                cell->bg = current_bg;
                draw_char_at(cursor_x, cursor_y, ' ', current_fg, current_bg);
                cursor_x++;
                if (cursor_x >= max_cols) {
                    cursor_x = 0;
                    cursor_y++;
                    if (cursor_y >= max_rows) {
                        scroll();
                        cursor_y = max_rows - 1;
                    }
                }
            }
        }
    } else if (c >= ' ') {
        if (cursor_x < max_cols && cursor_y < max_rows) {
            console_cell_t* cell = &text_buffer[cursor_y * max_cols + cursor_x];
            cell->c = c;
            cell->fg = current_fg;
            cell->bg = current_bg;
            draw_char_at(cursor_x, cursor_y, c, current_fg, current_bg);
        }
        cursor_x++;
    }
}

void fb_cons_putc(char c) {
    if (!gpu_get_primary() || !text_buffer) return;

    /* Hide cursor, draw char, show cursor */
    draw_cursor(0);
    fb_cons_putc_internal(c);
    draw_cursor(1);
}

void fb_cons_puts(const char* s) {
    if (!gpu_get_primary() || !text_buffer) return;

    /* Optimization: Hide cursor ONCE for the whole string */
    draw_cursor(0);
    while (*s) {
        fb_cons_putc_internal(*s++);
    }
    draw_cursor(1);
}

void fb_cons_clear(void) {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu || !text_buffer) return;
    serial("[FB_CONS] Clearing screen...\n");

    /* Clear text buffer */
    for (uint32_t i = 0; i < max_cols * max_rows; i++) {
        text_buffer[i].c = ' ';
        text_buffer[i].fg = current_fg;
        text_buffer[i].bg = current_bg;
    }
    
    /* Reset cursor */
    cursor_x = 0;
    cursor_y = 0;
    
    /* Clear screen and redraw cursor */
    gpu->ops->clear(gpu, current_bg);
    draw_cursor(1);
    serial("[FB_CONS] Clear done.\n");
}