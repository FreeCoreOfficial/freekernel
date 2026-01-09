#include "../terminal.h"
#include "../string.h"
#include "fat.h"
#include "cd.h"

extern "C" void cmd_ls(int argc, char** argv) {
    char cwd[256];
    cd_get_cwd(cwd, 256);
    
    char target[256];

    if (argc > 1) {
        const char* arg = argv[1];
        
        /* Handle absolute vs relative path */
        if (arg[0] == '/') {
            strcpy(target, arg);
        } else {
            strcpy(target, cwd);
            size_t len = strlen(target);
            if (len > 0 && target[len-1] != '/') {
                strcat(target, "/");
            }
            strcat(target, arg);
        }
    } else {
        strcpy(target, cwd);
    }

    /* List FAT32 directory */
    fat_automount();
    fat32_list_directory(target);
}
