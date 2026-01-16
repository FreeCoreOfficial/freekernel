/* os/kernel/apps/icons/icons.c */
#include "icons.h"
#include "../../mem/kmalloc.h"
#include "../../cmds/fat.h"
#include "../../fs/fs.h"
#include "../../string.h"
#include "../../drivers/serial.h"
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
    
    /* 1. Try FAT32 first (persistent storage) */
    serial_printf("[ICONS] Checking persistent storage at /system/icons.mod...\n");
    fat_automount();
    
    int32_t size = fat32_get_file_size("/system/icons.mod");
    if (size > 0) {
        data = kmalloc(size);
        if (data) {
            int read_result = fat32_read_file("/system/icons.mod", data, size);
            if (read_result == size) {
                file_size = (size_t)size;
                serial_printf("[ICONS] Loaded from FAT32: %u bytes\n", (unsigned)file_size);
            } else {
                serial_printf("[ICONS] FAT32 read failed, trying RAMFS...\n");
                kfree(data);
                data = NULL;
            }
        } else {
            serial_printf("[ICONS] kmalloc failed for %d bytes\n", size);
        }
    } else {
        serial_printf("[ICONS] Not in persistent storage, trying RAMFS...\n");
    }
    
    /* 2. Try RAMFS (multiboot module) if FAT32 failed */
    if (!data) {
        file_data = ramfs_read_file(path, &file_size);
        if (file_data && file_size > 0) {
            /* Use RAMFS data directly (bootloader reserves this memory) */
            data = (void*)file_data;
            serial_printf("[ICONS] Loaded from RAMFS: %u bytes at 0x%x\n", (unsigned)file_size, (unsigned)file_data);
        } else {
            serial_printf("[ICONS] Not found in RAMFS either\n");
            return false;
        }
    }
    
    /* Validate magic number */
    icons_mod_header_t* h = (icons_mod_header_t*)data;
    if (h->magic != ICONS_MAGIC) {
        serial_printf("[ICONS] Invalid magic: 0x%x (expected 0x%x)\n", h->magic, ICONS_MAGIC);
        if (file_data != data) kfree(data);  /* Only free if we allocated it */
        return false;
    }
    
    g_icons.count     = h->count;
    g_icons.entries   = (const icons_mod_entry_t*)(h + 1);
    g_icons.base      = (const uint8_t*)data;
    g_icons.file_data = data;
    
    serial_printf("[ICONS] Initialized successfully: %u icons\n", g_icons.count);
    return true;
}

const icon_image_t* icon_get(uint16_t id) {
    static icon_image_t img;
    static uint32_t converted_pixels[256 * 256]; /* Max 256x256 for conversion buffer */
    static uint16_t last_id = 0xFFFF;
    static const uint8_t* cached_rgba_src = NULL;

    if (!g_icons.base) {
        serial_printf("[ICONS] ERROR: g_icons.base is NULL\n");
        return 0;
    }

    for (int i = 0; i < g_icons.count; i++) {
        if (g_icons.entries[i].id == id) {
            img.w = g_icons.entries[i].w;
            img.h = g_icons.entries[i].h;
            
            /* Get source RGBA data */
            const uint8_t* rgba_src = (const uint8_t*)(g_icons.base + g_icons.entries[i].offset);
            size_t pixel_count = (size_t)(img.w * img.h);
            
            /* For large icons, we need to downscale in taskbar rendering, not here */
            /* Check if we can convert (limited by buffer size) */
            if (pixel_count > sizeof(converted_pixels) / sizeof(converted_pixels[0])) {
                /* Don't convert large icons here - return raw RGBA pointer wrapped as 32-bit */
                img.pixels = (const uint32_t*)rgba_src;
                return &img;
            }
            
            /* For smaller icons or if requested, convert RGBA to ARGB */
            if (last_id != id || cached_rgba_src == NULL) {
                for (size_t j = 0; j < pixel_count; j++) {
                    uint8_t r = rgba_src[j * 4 + 0];
                    uint8_t g = rgba_src[j * 4 + 1];
                    uint8_t b = rgba_src[j * 4 + 2];
                    uint8_t a = rgba_src[j * 4 + 3];
                    /* Convert RGBA to ARGB */
                    converted_pixels[j] = (a << 24) | (r << 16) | (g << 8) | b;
                }
                cached_rgba_src = rgba_src;
                last_id = id;
            }
            
            img.pixels = converted_pixels;
            return &img;
        }
    }
    
    serial_printf("[ICONS] Icon ID %d not found (total: %d)\n", id, g_icons.count);
    return 0;
}
