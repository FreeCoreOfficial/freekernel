/* os/kernel/apps/icons/icons.c */
#include "icons.h"
#include "../../mem/kmalloc.h"
#include "../../cmds/fat.h"
#include "../../fs/fs.h"
#include "../../string.h"
#include <stddef.h>

#define ICONS_MAGIC 0x4E4F4349  /* 'ICON' in ASCII */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
} icons_mod_header_t;

typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t w;
    uint16_t h;
    uint32_t offset;  /* Offset to pixel data relative to file start */
} icons_mod_entry_t;

static struct {
    uint16_t count;
    const icons_mod_entry_t* entries;
    const uint8_t* base;
    void* file_data;
} g_icons = {0, 0, 0, 0};

bool icons_init(const char* path) {
    const void* file_data = NULL;
    size_t file_size = 0;
    void* data = NULL;
    
    /* 1. Try RAMFS first (for multiboot modules) */
    file_data = ramfs_read_file(path, &file_size);
    if (file_data && file_size > 0) {
        /* Copy data to our own buffer (multiboot memory may be overwritten) */
        data = kmalloc(file_size);
        if (data) {
            memcpy(data, file_data, file_size);
        } else {
            return false;
        }
    } else {
        /* 2. Fallback to FAT32 */
        fat_automount();
        
        int32_t size = fat32_get_file_size(path);
        if (size <= 0) return false;
        
        data = kmalloc(size);
        if (!data) return false;
        
        if (fat32_read_file(path, data, size) != size) {
            kfree(data);
            return false;
        }
        file_size = (size_t)size;
    }
    
    /* Validate magic number */
    icons_mod_header_t* h = (icons_mod_header_t*)data;
    if (h->magic != ICONS_MAGIC) {
        kfree(data);
        return false;
    }
    
    g_icons.count     = h->count;
    g_icons.entries   = (const icons_mod_entry_t*)(h + 1);
    g_icons.base      = (const uint8_t*)data;
    g_icons.file_data = data;
    
    return true;
}

const icon_image_t* icon_get(uint16_t id) {
    static icon_image_t img;
    static uint32_t converted_pixels[256 * 256]; /* Max 256x256 icon */
    static uint16_t last_id = 0xFFFF;
    static uint32_t* cached_pixels = NULL;

    if (!g_icons.base) return 0;

    for (int i = 0; i < g_icons.count; i++) {
        if (g_icons.entries[i].id == id) {
            img.w = g_icons.entries[i].w;
            img.h = g_icons.entries[i].h;
            
            /* Convert RGBA8888 (from mkicons.py) to ARGB8888 (system format) */
            const uint8_t* rgba_src = (const uint8_t*)(g_icons.base + g_icons.entries[i].offset);
            size_t pixel_count = (size_t)(img.w * img.h);
            
            /* Use cached buffer if same icon requested */
            if (last_id != id || cached_pixels == NULL) {
                if (pixel_count <= sizeof(converted_pixels) / sizeof(converted_pixels[0])) {
                    for (size_t j = 0; j < pixel_count; j++) {
                        uint8_t r = rgba_src[j * 4 + 0];
                        uint8_t g = rgba_src[j * 4 + 1];
                        uint8_t b = rgba_src[j * 4 + 2];
                        uint8_t a = rgba_src[j * 4 + 3];
                        /* Convert RGBA to ARGB */
                        converted_pixels[j] = (a << 24) | (r << 16) | (g << 8) | b;
                    }
                    cached_pixels = converted_pixels;
                    last_id = id;
                } else {
                    return 0; /* Icon too large */
                }
            }
            
            img.pixels = cached_pixels;
            return &img;
        }
    }
    return 0;
}
