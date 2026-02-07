#pragma once
#include "../flyui.h"

#ifdef __cplusplus
extern "C" {
#endif

fly_widget_t *fly_label_create(const char *text);
void fly_label_set_text(fly_widget_t *w, const char *text);
fly_widget_t *fly_button_create(const char *text);
void fly_button_set_callback(fly_widget_t *w, fly_on_click_t on_click);

/* Panel helper */
fly_widget_t *fly_panel_create(int w, int h);

#ifdef __cplusplus
}
#endif
