#include "theme.h"

static fly_theme_t current_theme;

void theme_init(void) {
    /* Chrysalis Aero: cool slate + blue glass accents */
    current_theme.win_bg = 0xFFF5F7FA; /* light fog */

    current_theme.win_title_active_bg = 0xFF2B6AAE; /* aero blue */
    current_theme.win_title_active_fg = 0xFFFFFFFF;

    current_theme.win_title_inactive_bg = 0xFFB9C3CF; /* muted blue-gray */
    current_theme.win_title_inactive_fg = 0xFF2C3137;

    current_theme.color_hi_1 = 0xFFFFFFFF; /* highlight */
    current_theme.color_lo_1 = 0xFF9AA6B2; /* soft edge */
    current_theme.color_lo_2 = 0xFF2B3036; /* deep edge */
    current_theme.color_text = 0xFF1C2228;

    current_theme.border_width = 1;
    current_theme.title_height = 28;
    current_theme.padding = 8;
}

fly_theme_t* theme_get(void) {
    return &current_theme;
}
