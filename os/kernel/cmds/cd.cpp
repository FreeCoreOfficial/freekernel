#include "cd.h"
#include "fat.h"
#include "../terminal.h"
#include "../string.h"

static char cwd[256] = "/";

extern "C" void cd_get_cwd(char* buf, size_t size) {
    strncpy(buf, cwd, size);
    buf[size - 1] = 0;
}

/* Helper to normalize path (handle . and ..) */
static void normalize_path(char* path) {
    char* stack[32];
    int top = 0;
    
    /* We need a copy to tokenize because we will reconstruct into path */
    char temp[256];
    strncpy(temp, path, 256);
    
    char* p = temp;
    if (*p == '/') p++;
    
    char* token = p;
    while (*p) {
        if (*p == '/') {
            *p = 0;
            if (strcmp(token, "") != 0 && strcmp(token, ".") != 0) {
                if (strcmp(token, "..") == 0) {
                    if (top > 0) top--;
                } else {
                    if (top < 32) stack[top++] = token;
                }
            }
            token = p + 1;
        }
        p++;
    }
    if (strcmp(token, "") != 0 && strcmp(token, ".") != 0) {
        if (strcmp(token, "..") == 0) {
            if (top > 0) top--;
        } else {
            if (top < 32) stack[top++] = token;
        }
    }
    
    /* Reconstruct into original path */
    path[0] = '/';
    path[1] = 0;
    for (int i = 0; i < top; i++) {
        if (i > 0) strcat(path, "/");
        strcat(path, stack[i]);
    }
}

extern "C" int cmd_cd(int argc, char** argv) {
    /* If no args, go to root (like home) */
    const char* target = "/";
    if (argc >= 2) {
        target = argv[1];
    }

    fat_automount();

    /* Construct new path */
    char new_path[256];
    
    if (target[0] == '/') {
        /* Absolute */
        strncpy(new_path, target, 255);
    } else {
        /* Relative */
        strncpy(new_path, cwd, 255);
        int len = strlen(new_path);
        if (new_path[len-1] != '/') {
            strcat(new_path, "/");
        }
        strcat(new_path, target);
    }
    
    /* Normalize */
    normalize_path(new_path);
    
    /* Verify existence */
    /* Special case: root always exists */
    if (strcmp(new_path, "/") == 0) {
        strcpy(cwd, "/");
        return 0;
    }
    
    /* For other paths, we check the last component against the FAT driver.
       Note: Current FAT driver is flat-ish, so we check if the directory name exists.
       This is imperfect for nested dirs but works for 1-level depth. */
       
    /* Extract last component for check */
    char* last_slash = strchr(new_path, 0);
    while (last_slash > new_path && *last_slash != '/') last_slash--;
    const char* check_name = (last_slash && *last_slash == '/') ? last_slash + 1 : new_path;
    
    if (fat32_directory_exists(check_name)) {
        strcpy(cwd, new_path);
    } else {
        terminal_printf("cd: %s: No such directory\n", target);
        return -1;
    }

    return 0;
}