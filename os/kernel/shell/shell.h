#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void shell_init();
void shell_handle_char(char c);

/* API pentru shortcuts */
void shell_reset_input(void);
void shell_prompt(void);

#ifdef __cplusplus
}
#endif
