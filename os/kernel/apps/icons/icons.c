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
static int g_bmp_cursor = 0;

static icon_image_t g_placeholder_icon;
static uint32_t g_placeholder_pixels[16 * 16];
static bool g_placeholder_ready = false;

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

static const icon_image_t* icon_placeholder(void) {
    if (!g_placeholder_ready) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                bool border = (x == 0 || y == 0 || x == 15 || y == 15);
                uint32_t c = border ? 0xFF3A3F45 : 0xFFD6D0C6;
                g_placeholder_pixels[y * 16 + x] = c;
            }
        }
        g_placeholder_icon.w = 16;
        g_placeholder_icon.h = 16;
        g_placeholder_icon.pixels = g_placeholder_pixels;
        g_placeholder_ready = true;
    }
    return &g_placeholder_icon;
}

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
    uint8_t* data = (uint8_t*)kmalloc((size_t)size);
    if (!data) return false;
    int rd = fat32_read_file(path, data, (uint32_t)size);
    if (rd < size) {
        kfree(data);
        return false;
    }
    bool ok = load_bmp_from_mem(data, (size_t)size, out);
    kfree(data);
    return ok;
}

static void build_fat_icon_path(char* out, size_t cap, const char* name) {
    if (!out || cap == 0) return;
    /* "/system/icons/<name>" */
    out[0] = '/'; out[1] = 's'; out[2] = 'y'; out[3] = 's';
    out[4] = 't'; out[5] = 'e'; out[6] = 'm'; out[7] = '/';
    out[8] = 'i'; out[9] = 'c'; out[10] = 'o'; out[11] = 'n';
    out[12] = 's'; out[13] = '/';
    size_t i = 0;
    while (name[i] && (14 + i) < cap - 1) {
        out[14 + i] = name[i];
        i++;
    }
    out[14 + i] = 0;
}

static bool bmp_available(void) {
    size_t rsize = 0;
    const void* rdata = ramfs_read_file("start.bmp", &rsize);
    if (rdata && rsize > 0) return true;
    if (fat32_get_file_size("/system/icons/start.bmp") > 0) return true;
    return false;
}

static bool __attribute__((unused)) try_load_bmp_icons(void) {
    bool any = false;
    for (int i = 0; i < ICON_COUNT; i++) {
        g_bmp_loaded[i] = 0;
        g_bmp_icons[i].pixels = NULL;
        char fat_path[48];
        const char* name = g_icon_files[i];
        build_fat_icon_path(fat_path, sizeof(fat_path), name);

        /* RAMFS first: try "/<name>" */
        size_t rsize = 0;
        const void* rdata = ramfs_read_file(name, &rsize);
        if (rdata && rsize > 0) {
            if (load_bmp_from_mem((const uint8_t*)rdata, rsize, &g_bmp_icons[i])) {
                g_bmp_loaded[i] = 1;
                any = true;
                continue;
            }
        }

        if (load_bmp_from_fat(fat_path, &g_bmp_icons[i])) {
            g_bmp_loaded[i] = 1;
            any = true;
            continue;
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
    if (g_use_bmp) return true;
    if (g_icons.count > 0 && g_icons.entries) return true;
    const void* file_data = NULL;
    size_t file_size = 0;
    void* data = NULL;
    
    /* 0. Detect BMP icons (preferred) */
    fat_automount();
    if (bmp_available()) {
        for (int i = 0; i < ICON_COUNT; i++) {
            g_bmp_loaded[i] = 0;
            g_bmp_icons[i].pixels = NULL;
        }
        g_bmp_cursor = 0;
        g_use_bmp = true;
        serial_printf("[ICONS] BMP icons available (lazy loading).\n");
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

    if (g_use_bmp && id < ICON_COUNT) {
        if (g_bmp_loaded[id] == 1 && g_bmp_icons[id].pixels) {
            return &g_bmp_icons[id];
        }
        return icon_placeholder();
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

bool icons_tick(int max_to_load) {
    if (!g_use_bmp || max_to_load <= 0) return false;
    bool did_load = false;
    int loaded = 0;
    int scanned = 0;

    while (scanned < ICON_COUNT && loaded < max_to_load) {
        int id = g_bmp_cursor;
        g_bmp_cursor = (g_bmp_cursor + 1) % ICON_COUNT;
        scanned++;

        if (g_bmp_loaded[id] != 0) continue;

        const char* name = g_icon_files[id];
        size_t rsize = 0;
        const void* rdata = ramfs_read_file(name, &rsize);
        if (rdata && rsize > 0 && load_bmp_from_mem((const uint8_t*)rdata, rsize, &g_bmp_icons[id])) {
            g_bmp_loaded[id] = 1;
            did_load = true;
        } else {
            char fat_path[48];
            build_fat_icon_path(fat_path, sizeof(fat_path), name);
            if (load_bmp_from_fat(fat_path, &g_bmp_icons[id])) {
                g_bmp_loaded[id] = 1;
                did_load = true;
            } else {
                g_bmp_loaded[id] = 2;
            }
        }
        loaded++;
    }

    return did_load;
}
