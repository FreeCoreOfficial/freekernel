#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int cmd_fat(int argc, char **argv);

/* Încearcă să monteze automat prima partiție FAT32 găsită */
void fat_automount(void);

#ifdef __cplusplus
}
#endif
