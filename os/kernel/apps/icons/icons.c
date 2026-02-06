/* os/kernel/apps/icons/icons.c */
#include "icons.h"
#include "../../mem/kmalloc.h"
#include "../../cmds/fat.h"
#include "../../fs/fs.h"
#include "../../string.h"
#include "../../drivers/serial.h"
#include <stddef.h>

#define ICONS_MAGIC 0x4E4F4349  /* 'ICON' in ASCII */
#define ICON_COUNT 16

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;      /* 'BM' */
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

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

static icon_image_t g_bmp_icons[ICON_COUNT];
static uint8_t g_bmp_loaded[ICON_COUNT];
static bool g_use_bmp = false;

static const char* g_icon_files[ICON_COUNT] = {
    "start.bmp",
    "term.bmp",
    "files.bmp",
    "img.bmp",
    "note.bmp",
    "paint.bmp",
    "calc.bmp",
    "clock.bmp",
    "calc.bmp", /* ICON_CAL reuses calc */
    "task.bmp",
    "info.bmp",
    "3D.bmp",
    "mine.bmp",
    "net.bmp",
    "x0.bmp",
    "run.bmp",
};

static bool load_bmp_from_mem(const uint8_t* data, size_t size, icon_image_t* out) {
    if (!data || size < 54 || !out) return false;

    const BITMAPFILEHEADER* fileHeader = (const BITMAPFILEHEADER*)data;
    if (fileHeader->bfType != 0x4D42) return false;
    if (fileHeader->bfOffBits >= size) return false;

    const BITMAPINFOHEADER* infoHeader =
        (const BITMAPINFOHEADER*)(data + sizeof(BITMAPFILEHEADER));

    int32_t width = infoHeader->biWidth;
    int32_t height = infoHeader->biHeight;
    uint16_t bpp = infoHeader->biBitCount;
    if (width <= 0 || height == 0) return false;
    if (bpp != 24 && bpp != 32) return false;

    int absHeight = (height > 0) ? height : -height;
    int rowSize = ((width * bpp + 31) / 32) * 4;
    size_t pixelBytes = (size_t)width * (size_t)absHeight * 4;
    uint8_t* pixels = (uint8_t*)kmalloc(pixelBytes);
    if (!pixels) return false;

    const uint8_t* base = data + fileHeader->bfOffBits;
    for (int i = 0; i < absHeight; i++) {
        size_t rowOff = (size_t)i * (size_t)rowSize;
        if (fileHeader->bfOffBits + rowOff + rowSize > size) {
            kfree(pixels);
            return false;
        }
        const uint8_t* row = base + rowOff;
        int screenY = (height > 0) ? (absHeight - 1 - i) : i;
        for (int x = 0; x < width; x++) {
            uint8_t b = row[x * (bpp / 8) + 0];
            uint8_t g = row[x * (bpp / 8) + 1];
            uint8_t r = row[x * (bpp / 8) + 2];
            uint8_t a = (bpp == 32) ? row[x * 4 + 3] : 0xFF;
            size_t idx = ((size_t)screenY * (size_t)width + (size_t)x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    }

    out->w = (uint16_t)width;
    out->h = (uint16_t)absHeight;
    out->pixels = (const uint32_t*)pixels;
    return true;
}

static bool load_bmp_from_fat(const char* path, icon_image_t* out) {
    int32_t size = fat32_get_file_size(path);
    if (size <= 0) return false;
    uint8_t header[54];
    if (fat32_read_file(path, header, sizeof(header)) < (int)sizeof(header)) return false;
    BITMAPFILEHEADER* fileHeader = (BITMAPFILEHEADER*)header;
    BITMAPINFOHEADER* infoHeader = (BITMAPINFOHEADER*)(header + sizeof(BITMAPFILEHEADER));
    if (fileHeader->bfType != 0x4D42) return false;
    int32_t width = infoHeader->biWidth;
    int32_t height = infoHeader->biHeight;
    uint16_t bpp = infoHeader->biBitCount;
    if (width <= 0 || height == 0) return false;
    if (bpp != 24 && bpp != 32) return false;

    int absHeight = (height > 0) ? height : -height;
    int rowSize = ((width * bpp + 31) / 32) * 4;
    size_t pixelBytes = (size_t)width * (size_t)absHeight * 4;
    uint8_t* pixels = (uint8_t*)kmalloc(pixelBytes);
    if (!pixels) return false;

    uint8_t* rowBuffer = (uint8_t*)kmalloc(rowSize);
    if (!rowBuffer) { kfree(pixels); return false; }

    for (int i = 0; i < absHeight; i++) {
        uint32_t filePos = fileHeader->bfOffBits + i * rowSize;
        if (fat32_read_file_offset(path, rowBuffer, rowSize, filePos) != rowSize) {
            kfree(rowBuffer);
            kfree(pixels);
            return false;
        }
        int screenY = (height > 0) ? (absHeight - 1 - i) : i;
        for (int x = 0; x < width; x++) {
            uint8_t b = rowBuffer[x * (bpp / 8) + 0];
            uint8_t g = rowBuffer[x * (bpp / 8) + 1];
            uint8_t r = rowBuffer[x * (bpp / 8) + 2];
            uint8_t a = (bpp == 32) ? rowBuffer[x * 4 + 3] : 0xFF;
            size_t idx = ((size_t)screenY * (size_t)width + (size_t)x) * 4;
            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    }
    kfree(rowBuffer);
    out->w = (uint16_t)width;
    out->h = (uint16_t)absHeight;
    out->pixels = (const uint32_t*)pixels;
    return true;
}

static bool try_load_bmp_icons(void) {
    bool any = false;
    for (int i = 0; i < ICON_COUNT; i++) {
        g_bmp_loaded[i] = 0;
        g_bmp_icons[i].pixels = NULL;
        char fat_path[48];
        /* "/system/icons/<name>" */
        fat_path[0] = '/'; fat_path[1] = 's'; fat_path[2] = 'y'; fat_path[3] = 's';
        fat_path[4] = 't'; fat_path[5] = 'e'; fat_path[6] = 'm'; fat_path[7] = '/';
        fat_path[8] = 'i'; fat_path[9] = 'c'; fat_path[10] = 'o'; fat_path[11] = 'n';
        fat_path[12] = 's'; fat_path[13] = '/';
        const char* name = g_icon_files[i];
        int j = 0;
        while (name[j] && (14 + j) < (int)sizeof(fat_path) - 1) {
            fat_path[14 + j] = name[j];
            j++;
        }
        fat_path[14 + j] = 0;

        if (load_bmp_from_fat(fat_path, &g_bmp_icons[i])) {
            g_bmp_loaded[i] = 1;
            any = true;
            continue;
        }

        /* RAMFS fallback: try "/<name>" */
        size_t rsize = 0;
        const void* rdata = ramfs_read_file(name, &rsize);
        if (rdata && rsize > 0) {
            if (load_bmp_from_mem((const uint8_t*)rdata, rsize, &g_bmp_icons[i])) {
                g_bmp_loaded[i] = 1;
                any = true;
            }
        }
    }
    return any;
}

static bool load_icons_from_parts(void** out_data, size_t* out_size) {
    const int max_parts = 256;
    size_t total = 0;
    int parts = 0;
    char path[32];

    for (int i = 0; i < max_parts; i++) {
        int a = (i / 100) % 10;
        int b = (i / 10) % 10;
        int c = i % 10;
        /* "/system/icons/partXYZ" */
        path[0] = '/';
        path[1] = 's';
        path[2] = 'y';
        path[3] = 's';
        path[4] = 't';
        path[5] = 'e';
        path[6] = 'm';
        path[7] = '/';
        path[8] = 'i';
        path[9] = 'c';
        path[10] = 'o';
        path[11] = 'n';
        path[12] = 's';
        path[13] = '/';
        path[14] = 'p';
        path[15] = 'a';
        path[16] = 'r';
        path[17] = 't';
        path[18] = (char)('0' + a);
        path[19] = (char)('0' + b);
        path[20] = (char)('0' + c);
        path[21] = 0;

        int32_t sz = fat32_get_file_size(path);
        if (sz <= 0) break;
        total += (size_t)sz;
        parts++;
    }

    if (parts == 0 || total == 0) return false;

    void* data = kmalloc(total);
    if (!data) {
        serial_printf("[ICONS] kmalloc failed for %u bytes (parts)\n", (unsigned)total);
        return false;
    }

    size_t off = 0;
    for (int i = 0; i < parts; i++) {
        int a = (i / 100) % 10;
        int b = (i / 10) % 10;
        int c = i % 10;
        path[0] = '/';
        path[1] = 's';
        path[2] = 'y';
        path[3] = 's';
        path[4] = 't';
        path[5] = 'e';
        path[6] = 'm';
        path[7] = '/';
        path[8] = 'i';
        path[9] = 'c';
        path[10] = 'o';
        path[11] = 'n';
        path[12] = 's';
        path[13] = '/';
        path[14] = 'p';
        path[15] = 'a';
        path[16] = 'r';
        path[17] = 't';
        path[18] = (char)('0' + a);
        path[19] = (char)('0' + b);
        path[20] = (char)('0' + c);
        path[21] = 0;

        int32_t sz = fat32_get_file_size(path);
        if (sz <= 0) {
            serial_printf("[ICONS] Missing part %d at %s\n", i, path);
            kfree(data);
            return false;
        }
        int rd = fat32_read_file(path, (uint8_t*)data + off, (uint32_t)sz);
        if (rd != sz) {
            serial_printf("[ICONS] Read failed for %s (got %d/%d)\n", path, rd, sz);
            kfree(data);
            return false;
        }
        off += (size_t)sz;
    }

    *out_data = data;
    *out_size = total;
    serial_printf("[ICONS] Loaded from parts: %d files, %u bytes\n", parts, (unsigned)total);
    return true;
}

bool icons_init(const char* path) {
    const void* file_data = NULL;
    size_t file_size = 0;
    void* data = NULL;
    
    /* 0. Try BMP icons (preferred) */
    fat_automount();
    if (try_load_bmp_icons()) {
        g_use_bmp = true;
        serial_printf("[ICONS] Loaded BMP icons.\n");
        return true;
    }

    /* 1. Try FAT32 icons.mod */
    serial_printf("[ICONS] Checking persistent storage at /system/icons.mod...\n");
    
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
        serial_printf("[ICONS] Not in persistent storage, trying parts...\n");
    }

    /* 1.5 Try chunked parts in /system/icons/ */
    if (!data) {
        void* part_data = NULL;
        size_t part_size = 0;
        if (load_icons_from_parts(&part_data, &part_size)) {
            data = part_data;
            file_size = part_size;
        } else {
            serial_printf("[ICONS] Not in parts, trying RAMFS...\n");
        }
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

    if (g_use_bmp && id < ICON_COUNT && g_bmp_loaded[id] && g_bmp_icons[id].pixels) {
        return &g_bmp_icons[id];
    }

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
