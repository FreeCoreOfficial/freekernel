#include "win.h"
#include "../ui/wm/wm.h"
#include "../video/compositor.h"
#include "../shell/shell.h"
#include "../input/input.h"
#include "../drivers/serial.h"
#include "../terminal.h"

extern "C" void serial(const char *fmt, ...);

extern "C" int cmd_launch(int argc, char** argv) {
    (void)argc; (void)argv;
    
    serial("[WIN] Starting GUI environment...\n");

    /* 1. Initialize GUI Subsystems */
    /* Note: We re-init them to ensure clean state */
    compositor_init();
    wm_init();
    
    /* 2. Create Terminal Window (The "DOS Box") */
    shell_create_window();
    
    /* 3. Main GUI Loop */
    bool running = true;
    input_event_t ev;
    
    /* Clear screen once to remove text mode artifacts */
    wm_mark_dirty();
    wm_render();

    while (running) {
        /* Poll Input */
        while (input_pop(&ev)) {
            if (ev.type == INPUT_KEYBOARD && ev.pressed) {
                /* Pass keyboard to Shell (Active Window) */
                shell_handle_char((char)ev.keycode);
                
                /* F12 to Exit GUI */
                if (ev.keycode == 0x58) { 
                     running = false; 
                }
            }
        }
        
        /* Render GUI */
        wm_render();
        
        /* Check if shell window was closed (if we had a close button) */
        if (!shell_is_window_active()) {
            running = false;
        }
        
        asm volatile("hlt");
    }
    
    /* 4. Cleanup & Return to Text Mode */
    shell_destroy_window();
    serial("[WIN] GUI shutdown. Returning to text mode.\n");
    return 0;
}