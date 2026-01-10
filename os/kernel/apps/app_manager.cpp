#include "app_manager.h"
#include "../mem/kmalloc.h"
#include "../string.h"

static app_info_t* app_list = NULL;

void app_manager_init(void) {
    app_list = NULL;
}

void app_register(const char* name, window_t* win) {
    app_info_t* app = (app_info_t*)kmalloc(sizeof(app_info_t));
    if (!app) return;
    
    strncpy(app->name, name, 31);
    app->name[31] = 0;
    app->window = win;
    app->selected = false;
    
    app->next = app_list;
    app_list = app;
}

void app_unregister(window_t* win) {
    app_info_t* curr = app_list;
    app_info_t* prev = NULL;
    
    while (curr) {
        if (curr->window == win) {
            if (prev) {
                prev->next = curr->next;
            } else {
                app_list = curr->next;
            }
            kfree(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

app_info_t* app_get_list(void) {
    return app_list;
}