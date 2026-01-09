#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t cl_color_t;

enum {
    CL_BLACK = 0,
    CL_BLUE,
    CL_GREEN,
    CL_CYAN,
    CL_RED,
    CL_MAGENTA,
    CL_BROWN,
    CL_LIGHT_GRAY,
    CL_DARK_GRAY,
    CL_LIGHT_BLUE,
    CL_LIGHT_GREEN,
    CL_LIGHT_CYAN,
    CL_LIGHT_RED,
    CL_LIGHT_MAGENTA,
    CL_YELLOW,
    CL_WHITE
};

void       cl_init(void);
cl_color_t cl_make(uint8_t fg, uint8_t bg);
uint8_t    cl_fg(cl_color_t c);
uint8_t    cl_bg(cl_color_t c);

void       cl_set_default(cl_color_t c);
cl_color_t cl_default(void);

/* Helper for framebuffer to map logical color to ARGB */
uint32_t   cl_rgb(uint8_t c);

#ifdef __cplusplus
}
#endif