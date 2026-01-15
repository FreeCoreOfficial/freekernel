/* kernel/fs/ramfs/ramfs_add.c */
#include "../../fs/fs.h"
#include "../../mem/kmalloc.h"
#include "../../string.h"
#include <stddef.h>

/* Static RAMFS root node for file storage (separate from VFS ramfs) */
static FSNode ramfs_file_root = {0};

/* Funcție care returnează rădăcina RAMFS pentru fișiere */
static FSNode* ramfs_file_root_get(void) {
    if (ramfs_file_root.name[0] == 0) {
        /* Initialize root node */
        memset(&ramfs_file_root, 0, sizeof(FSNode));
        strcpy(ramfs_file_root.name, "/");
        ramfs_file_root.flags = FS_DIR;
        ramfs_file_root.children = NULL;
        ramfs_file_root.parent = NULL;
    }
    return &ramfs_file_root;
}

void ramfs_create_file(const char* name, const void* data, size_t len) {
    FSNode* root = ramfs_file_root_get();
    if (!root) return;

    /* Alocăm nodul nou */
    FSNode* file = (FSNode*)kmalloc(sizeof(FSNode));
    if (!file) return;
    
    memset(file, 0, sizeof(FSNode));
    strncpy(file->name, name, sizeof(file->name) - 1);
    file->name[sizeof(file->name) - 1] = 0;
    file->length = len;
    file->data = (void*)data; /* Pointăm direct la datele din modulul Multiboot */
    file->flags = FS_FILE; 
    
    /* Îl adăugăm la începutul listei de copii ai rădăcinii */
    file->next = root->children;
    root->children = file;
    file->parent = root;
}

/* Helper: Find and read file from RAMFS */
const void* ramfs_read_file(const char* name, size_t* out_size) {
    FSNode* root = ramfs_file_root_get();
    if (!root) return NULL;
    
    /* Skip leading slash if present */
    const char* search_name = name;
    if (name[0] == '/') search_name = name + 1;
    
    /* Search in root's children */
    FSNode* node = root->children;
    while (node) {
        if (node->flags == FS_FILE && strcmp(node->name, search_name) == 0) {
            if (out_size) *out_size = node->length;
            return node->data;
        }
        node = node->next;
    }
    
    return NULL;
}
