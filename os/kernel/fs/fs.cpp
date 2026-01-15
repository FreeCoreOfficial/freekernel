#include "fs.h"
#include "../terminal.h"

static FSNode nodes[32];
static int node_count = 0;

static int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int strlen(const char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

static char* strdup(const char* s) {
    static char buffer[4096];
    static int offset = 0;

    int len = strlen(s) + 1;
    char* p = &buffer[offset];
    for (int i = 0; i < len; i++)
        p[i] = s[i];

    offset += len;
    return p;
}

void fs_init() {
    fs_create("hello.txt", "Hello from Chrysalis FS!\n");
    fs_create("readme.txt", "This is an in-memory filesystem.\n");
}

void fs_create(const char* name, const char* content) {
    if (node_count >= 32) return;

    /* Copiem numele în bufferul fix din FSNode */
    char* dst_name = nodes[node_count].name;
    int i = 0;
    while (name[i] && i < (int)sizeof(nodes[node_count].name) - 1) {
        dst_name[i] = name[i];
        i++;
    }
    dst_name[i] = 0;

    nodes[node_count].data = strdup(content);
    nodes[node_count].length = strlen(content);
    node_count++;
}

void fs_list() {
    for (int i = 0; i < node_count; i++) {
        terminal_writestring(nodes[i].name);
        terminal_writestring("\n");
    }
}

const FSNode* fs_find(const char* name) {
    for (int i = 0; i < node_count; i++) {
        if (strcmp(nodes[i].name, name) == 0)
            return &nodes[i];
    }
    return nullptr;
}
