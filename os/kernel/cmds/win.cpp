#include "win.h"
#include "../ui/wm/wm.h"
#include "../video/compositor.h"
#include "../video/gpu.h"
#include "../input/input.h"
#include "../drivers/serial.h"
#include "../terminal.h"
#include "../ui/flyui/flyui.h"
#include "../ui/flyui/widgets/widgets.h"
#include "../ui/flyui/theme.h"
#include "../shell/shell.h"
#include "../ui/flyui/bmp.h"
#include "../apps/clock_app.h"
#include "../apps/calculator_app.h"
#include "../apps/notepad_app.h"
#include "../apps/calendar_app.h"
#include "../string.h"
#include "../time/clock.h"
#include "../ui/flyui/draw.h"

extern "C" void serial(const char *fmt, ...);

/* Program Manager State */
static window_t* taskbar_win = NULL;
static flyui_context_t* taskbar_ctx = NULL;
static bool is_gui_running = false;
static int taskbar_last_min = -1;

/* Button Handler */
/* Button Handler: Lansează Terminalul */
static bool terminal_btn_event(fly_widget_t* w, fly_event_t* e) {
    (void)w;
    
    /* Lansăm terminalul pe Mouse Up pentru a evita lansări multiple accidentale */
    if (e->type == FLY_EVENT_MOUSE_UP) {
        if (!shell_is_window_active()) {
            serial("[WIN] Launching Terminal Window...\n");
            shell_create_window();
            
            /* Activăm randarea textului în fereastra nou creată */
            terminal_set_rendering(true);
            
            /* Forțăm o redesenare pentru a afișa fereastra imediat */
            wm_mark_dirty();
        }
        return true;
    } 
    else if (e->type == FLY_EVENT_MOUSE_DOWN) {
        /* Consumăm evenimentul de click și redesenăm pentru efect vizual */
        wm_mark_dirty();
        return true;
    }
    return false;
}

/* Button Handler: Lansează Ceasul */
static bool clock_btn_event(fly_widget_t* w, fly_event_t* e) {
    (void)w;
    if (e->type == FLY_EVENT_MOUSE_UP) {
        clock_app_create();
        return true;
    }
    return false;
}

/* Button Handler: Lansează Calculatorul */
static bool calc_btn_event(fly_widget_t* w, fly_event_t* e) {
    (void)w;
    if (e->type == FLY_EVENT_MOUSE_UP) {
        calculator_app_create();
        return true;
    }
    return false;
}

/* Button Handler: Lansează Notepad */
static bool note_btn_event(fly_widget_t* w, fly_event_t* e) {
    (void)w;
    if (e->type == FLY_EVENT_MOUSE_UP) {
        notepad_app_create();
        return true;
    }
    return false;
}

/* Button Handler: Lansează Calendarul */
static bool cal_btn_event(fly_widget_t* w, fly_event_t* e) {
    (void)w;
    if (e->type == FLY_EVENT_MOUSE_UP) {
        calendar_app_create();
        return true;
    }
    return false;
}

/* Taskbar Clock Widget Draw */
static void taskbar_clock_draw(fly_widget_t* w, surface_t* surf, int x, int y) {
    /* Background */
    fly_draw_rect_fill(surf, x, y, w->w, w->h, w->bg_color);
    
    datetime t;
    time_get_local(&t);
    
    char time_str[16];
    char date_str[16];
    char tmp[8];
    
    /* Time: HH:MM */
    time_str[0] = '0' + (t.hour / 10);
    time_str[1] = '0' + (t.hour % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (t.minute / 10);
    time_str[4] = '0' + (t.minute % 10);
    time_str[5] = 0;
    
    /* Date: DD.MM.YYYY */
    date_str[0] = '0' + (t.day / 10);
    date_str[1] = '0' + (t.day % 10);
    date_str[2] = '.';
    date_str[3] = '0' + (t.month / 10);
    date_str[4] = '0' + (t.month % 10);
    date_str[5] = '.';
    
    itoa_dec(tmp, t.year);
    strcpy(date_str + 6, tmp);
    
    /* Draw Time (Top) */
    int time_w = strlen(time_str) * 8;
    int tx = x + (w->w - time_w) / 2;
    int ty = y + 4;
    fly_draw_text(surf, tx, ty, time_str, 0xFF000000);
    
    /* Draw Date (Bottom) */
    int date_w = strlen(date_str) * 8;
    int dx = x + (w->w - date_w) / 2;
    int dy = y + 20;
    fly_draw_text(surf, dx, dy, date_str, 0xFF000000);
}

static void create_taskbar() {
    gpu_device_t* gpu = gpu_get_primary();
    if (!gpu) return;

    int w = gpu->width;
    int h = 40; /* Taskbar height */
    
    /* 1. Create Surface */
    surface_t* s = surface_create(w, h);
    if (!s) return;
    
    /* Solid gray background for taskbar */
    surface_clear(s, 0xFFC0C0C0);

    /* 2. Init FlyUI */
    taskbar_ctx = flyui_init(s);
    
    /* 3. Create Widgets */
    fly_widget_t* root = fly_panel_create(w, h);
    root->bg_color = 0xFFC0C0C0;
    flyui_set_root(taskbar_ctx, root);

    int x = 10;
    int y = 5;
    int bw = 90;
    int bh = 30;
    int gap = 10;

    /* Terminal Button */
    fly_widget_t* btn = fly_button_create("Terminal");
    btn->x = x; btn->y = y; btn->w = bw; btn->h = bh;
    btn->on_event = terminal_btn_event;
    fly_widget_add(root, btn);
    x += bw + gap;

    /* Clock Button */
    fly_widget_t* btn_clk = fly_button_create("Clock");
    btn_clk->x = x; btn_clk->y = y; btn_clk->w = bw; btn_clk->h = bh;
    btn_clk->on_event = clock_btn_event;
    fly_widget_add(root, btn_clk);
    x += bw + gap;

    /* Calculator Button */
    fly_widget_t* btn_calc = fly_button_create("Calc");
    btn_calc->x = x; btn_calc->y = y; btn_calc->w = bw; btn_calc->h = bh;
    btn_calc->on_event = calc_btn_event;
    fly_widget_add(root, btn_calc);
    x += bw + gap;

    /* Notepad Button */
    fly_widget_t* btn_note = fly_button_create("Notepad");
    btn_note->x = x; btn_note->y = y; btn_note->w = bw; btn_note->h = bh;
    btn_note->on_event = note_btn_event;
    fly_widget_add(root, btn_note);
    x += bw + gap;

    /* Calendar Button */
    fly_widget_t* btn_cal = fly_button_create("Calendar");
    btn_cal->x = x; btn_cal->y = y; btn_cal->w = bw; btn_cal->h = bh;
    btn_cal->on_event = cal_btn_event;
    fly_widget_add(root, btn_cal);
    x += bw + gap;

    /* Clock Widget (Right Aligned) */
    fly_widget_t* sys_clock = fly_panel_create(100, h);
    sys_clock->x = w - 105; /* 5px margin from right */
    sys_clock->y = 0;
    sys_clock->bg_color = 0xFFC0C0C0;
    sys_clock->on_draw = taskbar_clock_draw;
    fly_widget_add(root, sys_clock);

    /* 4. Initial Render */
    flyui_render(taskbar_ctx);

    /* 5. Create WM Window */
    /* Position at bottom of screen */
    taskbar_win = wm_create_window(s, 0, gpu->height - h);
}

extern "C" int cmd_launch_exit(int argc, char** argv) {
    (void)argc; (void)argv;
    if (is_gui_running) {
        is_gui_running = false;
        return 0;
    }
    terminal_writestring("GUI is not running.\n");
    return 1;
}

extern "C" int cmd_launch(int argc, char** argv) {
    (void)argc; (void)argv;
    
    if (is_gui_running) {
        terminal_writestring("GUI is already running.\n");
        return 1;
    }
    is_gui_running = true;
    
    serial("[WIN] Starting GUI environment...\n");

    /* 1. Disable Terminal Rendering (Text Mode OFF) - until shell window is opened */
    terminal_set_rendering(false);

    /* 2. Initialize GUI Subsystems */
    compositor_init();
    wm_init();
    
    /* 3. Create Taskbar */
    create_taskbar();
    
    /* 4. Main GUI Loop */
    input_event_t ev;
    
    /* Force initial render */
    wm_mark_dirty();
    wm_render();

    /* State for Window Dragging */
    window_t* drag_win = NULL;
    int drag_off_x = 0;
    int drag_off_y = 0;

    while (is_gui_running) {
        /* Update Apps */
        clock_app_update();

        /* Update Taskbar Clock */
        datetime t;
        time_get_local(&t);
        if (t.minute != taskbar_last_min) {
            taskbar_last_min = t.minute;
            /* Redraw taskbar to update clock */
            flyui_render(taskbar_ctx);
            wm_mark_dirty();
        }

        /* Poll Input */
        while (input_pop(&ev)) {
            /* Handle Global Keys */
            if (ev.type == INPUT_KEYBOARD && ev.pressed) {
                if (ev.keycode == 0x58) { /* F12 to Exit */
                     is_gui_running = false;
                }
                
                /* Route keyboard to Shell if it's active and focused (or if progman isn't focused) */
                window_t* focused = wm_get_focused();
                if (focused == shell_get_window()) {
                     shell_handle_char((char)ev.keycode);
                } else if (focused == notepad_app_get_window()) {
                     notepad_app_handle_key((char)ev.keycode);
                } 
            }
            
            /* Mouse Event Handling */
            if (ev.type == INPUT_MOUSE_MOVE || ev.type == INPUT_MOUSE_CLICK) {
                
                /* Handle Dragging Logic */
                if (drag_win && ev.type == INPUT_MOUSE_MOVE) {
                    drag_win->x = ev.mouse_x - drag_off_x;
                    drag_win->y = ev.mouse_y - drag_off_y;
                    wm_mark_dirty();
                }

                /* 1. Find Window Under Mouse (Top-most) if not dragging */
                window_t* target = drag_win ? drag_win : wm_find_window_at(ev.mouse_x, ev.mouse_y);

                /* 2. Handle Focus & Drag Start on Click */
                if (ev.type == INPUT_MOUSE_CLICK) {
                    if (ev.pressed) {
                        if (target) {
                            wm_focus_window(target);
                            wm_mark_dirty();
                            
                            /* Check for Title Bar Hit (0-24px relative to window) */
                            int rel_y = ev.mouse_y - target->y;
                            if (rel_y >= 0 && rel_y < 24) {
                                drag_win = target;
                                drag_off_x = ev.mouse_x - target->x;
                                drag_off_y = ev.mouse_y - target->y;
                            }
                        }
                    } else {
                        /* Mouse Up: Stop Dragging */
                        drag_win = NULL;
                    }
                }

                /* 3.1 Dispatch to Clock App */
                if (target == clock_app_get_window()) {
                    clock_app_handle_event(&ev);
                }

                /* 3.2 Dispatch to Shell Window */
                if (target == shell_get_window()) {
                    if (shell_handle_event(&ev)) {
                        target = NULL; /* Window destroyed */
                    }
                }

                /* 3.3 Dispatch to Calculator */
                if (target == calculator_app_get_window()) {
                    calculator_app_handle_event(&ev);
                }

                /* 3.4 Dispatch to Notepad */
                if (target == notepad_app_get_window()) {
                    notepad_app_handle_event(&ev);
                }

                /* 3.5 Dispatch to Calendar */
                if (target == calendar_app_get_window()) {
                    calendar_app_handle_event(&ev);
                }

                /* 3. Dispatch to Taskbar (only if it's the target and we are NOT dragging) */
                if (target == taskbar_win && taskbar_ctx && !drag_win) {
                    fly_event_t fev;
                    fev.mx = ev.mouse_x - taskbar_win->x;
                    fev.my = ev.mouse_y - taskbar_win->y;
                    fev.keycode = 0;
                    fev.type = FLY_EVENT_NONE;

                    if (ev.type == INPUT_MOUSE_MOVE) {
                        fev.type = FLY_EVENT_MOUSE_MOVE;
                    } else if (ev.type == INPUT_MOUSE_CLICK) {
                        fev.type = ev.pressed ? FLY_EVENT_MOUSE_DOWN : FLY_EVENT_MOUSE_UP;
                    }

                    if (fev.type != FLY_EVENT_NONE) {
                        flyui_dispatch_event(taskbar_ctx, &fev);
                        
                        /* Only redraw on interaction (clicks), NOT on mouse move.
                           This prevents the "Rendering" spam. */
                        if (fev.type != FLY_EVENT_MOUSE_MOVE) {
                            flyui_render(taskbar_ctx);
                            wm_mark_dirty();
                        }
                    }
                } else if (target == NULL) {
                    /* Clicked on background/desktop */
                    if (ev.type == INPUT_MOUSE_CLICK && ev.pressed) {
                        wm_mark_dirty();
                    }
                }
            }
        }
        
        /* Render GUI */
        wm_render();
        
        asm volatile("hlt");
    }
    
    /* 5. Cleanup & Return to Text Mode */
    if (taskbar_win) wm_destroy_window(taskbar_win);
    taskbar_win = NULL;
    taskbar_ctx = NULL;
    
    /* Dacă terminalul a fost deschis, îl închidem curat */
    if (shell_is_window_active()) {
        shell_destroy_window();
    }
    
    terminal_set_rendering(true);
    terminal_clear(); /* Clear artifacts */
    serial("[WIN] GUI shutdown. Returning to text mode.\n");
    is_gui_running = false;
    return 0;
}