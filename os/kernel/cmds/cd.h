#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int cmd_cd(int argc, char** argv);

/* Get current working directory */
void cd_get_cwd(char* buf, size_t size);

#ifdef __cplusplus
}
#endif