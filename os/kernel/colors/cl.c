#include "cl.h"
#include "../drivers/serial.h"

/* Import serial logging from kernel glue */
extern void serial(const char *fmt, ...);

static cl_color_t g_default_color = (CL_BLACK << 4) | CL_LIGHT_GRAY;

/* VGA Text Mode Palette (approximate RGB values) */
static const uint32_t palette[16] = {
    0xFF000000, // Black
    0xFF0000AA, // Blue
    0xFF00AA00, // Green
    0xFF00AAAA, // Cyan
    0xFFAA0000, // Red
    0xFFAA00AA, // Magenta
    0xFFAA5500, // Brown
    0xFFAAAAAA, // Light Gray
    0xFF555555, // Dark Gray
    0xFF5555FF, // Light Blue
    0xFF55FF55, // Light Green
    0xFF55FFFF, // Light Cyan
    0xFFFF5555, // Light Red
    0xFFFF55FF, // Light Magenta
    0xFFFFFF55, // Yellow
    0xFFFFFFFF  // White
};

void cl_init(void) {
    serial("[COLORS] Subsystem initialized.\n");
    /* Default: Light Gray on Black */
    g_default_color = cl_make(CL_LIGHT_GRAY, CL_BLACK);
}

cl_color_t cl_make(uint8_t fg, uint8_t bg) {
    return (cl_color_t)((bg << 4) | (fg & 0x0F));
}

uint8_t cl_fg(cl_color_t c) {
    return c & 0x0F;
}

uint8_t cl_bg(cl_color_t c) {
    return (c >> 4) & 0x0F;
}

void cl_set_default(cl_color_t c) {
    g_default_color = c;
}

cl_color_t cl_default(void) {
    return g_default_color;
}

uint32_t cl_rgb(uint8_t c) {
    return palette[c & 0x0F];
}