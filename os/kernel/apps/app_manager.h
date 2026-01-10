#pragma once
#include "../ui/wm/window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_info {
    char name[32];
    window_t* window;
    bool selected;
    struct app_info* next;
} app_info_t;

void app_manager_init(void);
void app_register(const char* name, window_t* win);
void app_unregister(window_t* win);
app_info_t* app_get_list(void);

#ifdef __cplusplus
}
#endif