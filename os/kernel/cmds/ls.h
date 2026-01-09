#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Lists files on the mounted FAT32 partition. Usage: ls [path] */
void cmd_ls(int argc, char** argv);

#ifdef __cplusplus
}
#endif
