#include "doom_app.h"
#include "../games/doom/doomgeneric/doomgeneric.h"
#include "../ui/wm/wm.h"
#include "../include/setjmp.h"

/* Defined in doomgeneric_chrysalis.c */
extern "C" void doom_key_push(int pressed, unsigned char key);
extern "C" window_t* doom_get_window(void);
extern "C" void doom_destroy_window(void);

static bool doom_running = false;

/* Defined in posix_stubs.c */
extern "C" jmp_buf exit_jmp_buf;
extern "C" int exit_jmp_set;

void doom_app_create(void) {
    if (doom_running) return;
    
    /* Arguments for Doom */
    /* Force doom1.wad to ensure it hits the embedded file check */
    char* argv[] = { (char*)"doom", (char*)"-iwad", (char*)"doom1.wad", NULL };
    
    /* Set recovery point for exit() */
    exit_jmp_set = 1;
    if (setjmp(exit_jmp_buf) == 0) {
        /* Initialize Doom */
        doomgeneric_Create(1, argv);
        doom_running = true;
    } else {
        /* Returned from exit() via longjmp */
        doom_running = false;
        doom_destroy_window();
        exit_jmp_set = 0;
    }
    /* Clear flag if we return normally (though doomgeneric_Create might not return if it loops, 
       but here we assume it returns or we rely on Tick) */
}

void doom_app_update(void) {
    if (!doom_running) return;
    if (!doom_get_window()) {
        doom_running = false;
        return;
    }
    
    /* Set recovery point for errors during Tick */
    exit_jmp_set = 1;
    if (setjmp(exit_jmp_buf) == 0) {
        doomgeneric_Tick();
    } else {
        doom_running = false;
        doom_destroy_window();
        exit_jmp_set = 0;
    }
}

bool doom_app_handle_event(input_event_t* ev) {
    window_t* win = doom_get_window();
    if (!win) return false;
    
    if (ev->type == INPUT_MOUSE_CLICK && ev->pressed) {
        int lx = ev->mouse_x - win->x;
        int ly = ev->mouse_y - win->y;
        
        /* Close Button (Top Right) */
        /* Assuming standard window layout from other apps */
        if (lx >= win->w - 20 && ly < 24) {
            doom_destroy_window();
            doom_running = false;
            return true;
        }
    }
    
    if (ev->type == INPUT_KEYBOARD) {
        /* Map Chrysalis Keys to Doom Keys */
        unsigned char key = 0;
        uint32_t k = ev->keycode;
        
        /* Basic mapping */
        if (k >= 'a' && k <= 'z') key = k;
        else if (k >= 'A' && k <= 'Z') key = k + 32; // to lower
        else if (k >= '0' && k <= '9') key = k;
        else if (k == 0x48) key = 0xAD; // Up Arrow (KEY_UPARROW)
        else if (k == 0x50) key = 0xAF; // Down Arrow (KEY_DOWNARROW)
        else if (k == 0x4B) key = 0xAC; // Left Arrow (KEY_LEFTARROW)
        else if (k == 0x4D) key = 0xAE; // Right Arrow (KEY_RIGHTARROW)
        else if (k == 0x1C) key = 13;   // Enter
        else if (k == 0x39) key = ' ';  // Space
        else if (k == 0x1D) key = 0x80; // Ctrl (Fire)
        else if (k == 0x01) key = 27;   // Esc
        
        if (key) {
            doom_key_push(ev->pressed ? 1 : 0, key);
            return true;
        }
    }
    
    return false;
}

window_t* doom_app_get_window(void) {
    return doom_get_window();
}