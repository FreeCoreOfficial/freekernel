#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool shortcuts_handle_ctrl(uint8_t scancode);
bool shortcuts_poll_ctrl_c(void);
void shortcuts_clear_ctrl_c(void);
void shortcuts_init(void);

/* --- new: notify by character + poll lowercase 'l' --- */
void shortcuts_notify_char(char c);
bool shortcuts_poll_key_l(void);
void shortcuts_clear_key_l(void);

#ifdef __cplusplus
}
#endif

#endif /* SHORTCUTS_H */
