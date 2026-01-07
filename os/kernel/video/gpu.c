/* kernel/video/gpu.c */
#include "gpu.h"
#include <stddef.h>

/* Import serial logging */
extern void serial(const char *fmt, ...);

/* Global GPU State */
static gpu_device_t* gpu_list_head = NULL;
static gpu_device_t* gpu_primary = NULL;
static uint32_t next_device_id = 0;

void gpu_init(void) {
    serial("[GPU] Core subsystem initialized.\n");
    gpu_list_head = NULL;
    gpu_primary = NULL;
    next_device_id = 0;
}

void gpu_register_device(gpu_device_t* dev) {
    if (!dev) return;

    dev->device_id = next_device_id++;
    dev->next = NULL;

    serial("[GPU] Registering device ID %d (Type: %d)\n", dev->device_id, dev->type);
    serial("[GPU]   Resolution: %dx%dx%d\n", dev->width, dev->height, dev->bpp);
    serial("[GPU]   Address: Phys=0x%x Virt=0x%x\n", dev->phys_addr, dev->virt_addr);

    /* Add to list */
    if (!gpu_list_head) {
        gpu_list_head = dev;
    } else {
        gpu_device_t* current = gpu_list_head;
        while (current->next) {
            current = current->next;
        }
        current->next = dev;
    }

    /* Select primary if none selected */
    if (!gpu_primary) {
        gpu_primary = dev;
        serial("[GPU] Primary GPU set to device ID %d.\n", dev->device_id);
    }
}

gpu_device_t* gpu_get_primary(void) {
    return gpu_primary;
}