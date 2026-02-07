#include "theme.h"

static fly_theme_t current_theme;

void theme_init(void) {
    /* Chrysalis Theme: warm paper + deep teal accents */
    current_theme.win_bg = 0xFFF2EFEA; /* soft warm paper */

    current_theme.win_title_active_bg = 0xFF1F6F78; /* deep teal */
    current_theme.win_title_active_fg = 0xFFFDF8F0; /* warm white */

    current_theme.win_title_inactive_bg = 0xFFB9C1C7; /* muted cool gray */
    current_theme.win_title_inactive_fg = 0xFF37414A;

    current_theme.color_hi_1 = 0xFFFFFFFF; /* highlight */
    current_theme.color_lo_1 = 0xFFCBD2D9; /* soft edge */
    current_theme.color_lo_2 = 0xFF3A3F45; /* deep edge */
    current_theme.color_text = 0xFF1E2429;

    current_theme.border_width = 1;
    current_theme.title_height = 26;
    current_theme.padding = 6;
}

fly_theme_t* theme_get(void) {
    return &current_theme;
}
