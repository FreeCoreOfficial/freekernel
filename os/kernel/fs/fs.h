#pragma once
#include <stdint.h>

struct FSNode {
    const char* name;
    char* data;
    uint32_t size;
};

void fs_init();
void fs_list();
const FSNode* fs_find(const char* name);
void fs_create(const char* name, const char* content);
