#include "doomgeneric.h"
#include "../../../ui/wm/wm.h"
#include "../../../ui/flyui/draw.h"
#include "../../../input/input.h"
#include "../../../mem/kmalloc.h"
#include "../../../cmds/fat.h"
#include "../../../string.h"
#include "../../../terminal.h"
#include "../../../apps/app_manager.h"

/* Networking stubs for Doom */
int drone = 0;
int net_client_connected = 0;

extern void serial(const char *fmt, ...);

/* Timer helper */
extern uint64_t hpet_time_ms(void);
uint32_t timer_get_ticks(void) { return (uint32_t)(hpet_time_ms() / 10); }

static window_t* doom_window = NULL;

/* Input Queue */
#define KEY_QUEUE_SIZE 32
static struct {
    int pressed;
    unsigned char key;
} key_queue[KEY_QUEUE_SIZE];
static int key_head = 0;
static int key_tail = 0;

void doom_key_push(int pressed, unsigned char key) {
    int next = (key_head + 1) % KEY_QUEUE_SIZE;
    if (next != key_tail) {
        key_queue[key_head].pressed = pressed;
        key_queue[key_head].key = key;
        key_head = next;
    }
}

/* --- DoomGeneric Implementation --- */

void DG_Init() {
    /* Create Window */
    surface_t* s = surface_create(DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    if (!s) return;
    
    /* Center on screen */
    doom_window = wm_create_window(s, 100, 100);
    
    /* Register app for Task Manager */
    app_register("DOOM", doom_window);
    
    terminal_printf("[DOOM] Window initialized.\n");
}

void DG_DrawFrame() {
    if (!doom_window) return;
    
    /* Blit buffer to window surface */
    /* DoomGeneric uses XRGB or similar. Chrysalis uses ARGB/BGRA (32bpp). */
    /* Assuming pixel_t is uint32_t (defined in doomgeneric.h) */
    /* DG_ScreenBuffer is allocated in doomgeneric.c */

    if (!DG_ScreenBuffer) return;

    memcpy(doom_window->surface->pixels, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
    
    wm_mark_dirty();
}

void DG_SleepMs(uint32_t ms) {
    /* Busy wait or yield (we don't have sleep() exposed easily here without headers) */
    /* Using a simple loop for now as we are in a cooperative loop anyway */
    uint32_t start = timer_get_ticks();
    while ((timer_get_ticks() - start) * 10 < ms) {
        asm volatile("nop");
    }
}

uint32_t DG_GetTicksMs() {
    return (uint32_t)hpet_time_ms();
}

int DG_GetKey(int* pressed, unsigned char* key) {
    if (key_head == key_tail) return 0;
    
    *pressed = key_queue[key_tail].pressed;
    *key = key_queue[key_tail].key;
    
    key_tail = (key_tail + 1) % KEY_QUEUE_SIZE;
    return 1;
}

void DG_SetWindowTitle(const char * title) {
    (void)title;
    /* Window title is drawn by WM, currently static or managed by app_create */
}

/* --- Window Access for App Wrapper --- */
window_t* doom_get_window(void) {
    return doom_window;
}

void doom_destroy_window(void) {
    if (doom_window) {
        wm_destroy_window(doom_window);
        app_unregister(doom_window);
        doom_window = NULL;
    }
}

/* --- Minimal C Library Wrappers for Doom --- */
/* These are needed because Doom uses stdio/stdlib */

void* malloc(size_t size) { return kmalloc(size); }
void* calloc(size_t nmemb, size_t size) {
    void* ptr = kmalloc(nmemb * size);
    if (ptr) memset(ptr, 0, nmemb * size);
    return ptr;
}
void* realloc(void* ptr, size_t size) {

    void* new_ptr = kmalloc(size);
    if (ptr && new_ptr) {
        memcpy(new_ptr, ptr, size); // DANGEROUS: might read OOB
        kfree(ptr);
    }
    return new_ptr;
}
void free(void* ptr) { kfree(ptr); }

int printf(const char* fmt, ...) {

    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    terminal_vprintf(fmt, &args);
    __builtin_va_end(args);
    return 0;
}

int puts(const char* s) {
    terminal_writestring(s);
    terminal_putchar('\n');
    return 0;
}

/* File I/O functions moved to kernel/posix_stubs.c */