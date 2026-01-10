#pragma once
#include "../ui/wm/window.h"
#include "../input/input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Creează fereastra ceasului */
void clock_app_create(void);

/* Actualizează ceasul (redesenează dacă s-a schimbat secunda) - apelat din bucla principală */
void clock_app_update(void);

/* Gestionează evenimentele (click, drag) pentru fereastra ceasului */
bool clock_app_handle_event(input_event_t* ev);

/* Returnează pointerul ferestrei pentru verificare în win.cpp */
window_t* clock_app_get_window(void);

#ifdef __cplusplus
}
#endif