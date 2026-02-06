#include <stddef.h>
#include <stdint.h>

#include "../os/kernel/arch/i386/io.h" /* for outb */
#include "../os/kernel/cmds/disk.h"    /* For disk_init, disk_read_sector etc */
#include "../os/kernel/cmds/fat.h"
#include "../os/kernel/drivers/serial.h"
#include "../os/kernel/fs/fat/fat.h"
#include "../os/kernel/fs/ramfs/ramfs.h"
#include "../os/kernel/include/types.h"
#include "../os/kernel/mem/kmalloc.h"
#include "../os/kernel/smp/multiboot.h"
#include "../os/kernel/storage/ata.h"
#include "../os/kernel/string.h"

/* Local BPB struct for installer debug dump */
struct fat_bpb {
  uint8_t jmp[3];
  char oem[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fats_count;
  uint16_t root_entries_count;
  uint16_t total_sectors_16;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat_16;
  uint16_t sectors_per_track;
  uint16_t heads_count;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
  uint32_t sectors_per_fat_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info;
  uint16_t backup_boot_sector;
  uint8_t reserved[12];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
} __attribute__((packed));

/* Helper Prototypes */
extern "C" {
void kmalloc_init(void);
void kmalloc_reset(void);
int fat32_format(uint32_t lba, uint32_t sector_count, const char *label);
int fat32_create_directory(const char *path);
int fat32_create_directory_verified(const char *path, int verify);
int fat32_create_file(const char *path, const void *data, uint32_t size);
int fat32_create_file_verified(const char *path, const void *data,
                               uint32_t size, int verify);
void fat32_set_mounted(uint32_t lba, char letter);
int fat32_read_directory(const char *path, fat_file_info_t *out,
                         int max_entries);
int32_t fat32_get_file_size(const char *path);
int fat32_create_file_alloc(const char *path, uint32_t size);
int fat32_write_file_offset(const char *path, const void *data, uint32_t size,
                            uint32_t offset, int verify);
void serial(const char *fmt, ...);
void ata_init(void);
void ata_set_skip_cache_flush(int enabled);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
int fat32_init(int port, uint64_t part_lba);
uint32_t disk_get_capacity(void);
int disk_write_sector(uint32_t lba, const uint8_t *buffer);
int disk_read_sector(uint32_t lba, uint8_t *buffer);
void ata_set_allow_mbr_write(int allow);
}

/* Icon filenames (BMP) */
#define ICON_COUNT 16
static const char* g_icon_files[ICON_COUNT] = {
    "start.bmp",
    "term.bmp",
    "files.bmp",
    "img.bmp",
    "note.bmp",
    "paint.bmp",
    "calc.bmp",
    "clock.bmp",
    "calc.bmp",
    "task.bmp",
    "info.bmp",
    "3D.bmp",
    "mine.bmp",
    "net.bmp",
    "x0.bmp",
    "run.bmp",
};

static void normalize_module_name_len(const char* cmdline, size_t cmdline_len, char* out, size_t out_sz) {
  if (!out || out_sz == 0) return;
  out[0] = 0;
  if (!cmdline) return;

  const char* s = cmdline;
  const char* end = cmdline + cmdline_len;
  while (s < end && (unsigned char)*s <= ' ') s++;
  while (end > s && (unsigned char)end[-1] <= ' ') end--;
  while (end > s && (unsigned char)end[-1] <= ' ') end--;

  const char* token = s;
  for (const char* p = s; p < end; ++p) {
    if (*p == ' ') token = p + 1;
  }

  if (*token == '\'' || *token == '"') {
    char q = *token++;
    if (end > token && end[-1] == q) end--;
  }

  const char* last = token;
  for (const char* p = token; p < end; ++p) {
    if (*p == '/') last = p + 1;
  }

  size_t n = (size_t)(end - last);
  if (n >= out_sz) n = out_sz - 1;
  memcpy(out, last, n);
  out[n] = 0;
}

static void write_sectors(uint32_t lba, const void *data, uint32_t bytes) {
  const uint8_t *p = (const uint8_t *)data;
  uint8_t *tmp = (uint8_t *)kmalloc(512);
  if (!tmp)
    return;
  uint32_t sectors = (bytes + 511) / 512;
  for (uint32_t i = 0; i < sectors; i++) {
    uint32_t copy = 512;
    if ((i + 1) * 512 > bytes)
      copy = bytes - i * 512;
    memset(tmp, 0, 512);
    memcpy(tmp, p + i * 512, copy);
    if (disk_write_sector(lba + i, tmp) != 0) {
      serial("[INSTALLER] ERROR: disk_write_sector failed at LBA %u\n",
             lba + i);
      break;
    }
  }
  kfree(tmp);
}

static void build_mbr(uint8_t *mbr, const uint8_t *boot_img, uint32_t start_lba,
                      uint32_t total_sectors) {
  memset(mbr, 0, 512);
  if (boot_img) {
    memcpy(mbr, boot_img, 446); /* boot code only */
  }
  /* Partition entry 0 */
  uint8_t *p = mbr + 446;
  p[0] = 0x80; /* bootable */
  p[1] = 0;
  p[2] = 0;
  p[3] = 0;    /* CHS start (unused) */
  p[4] = 0x0C; /* FAT32 LBA */
  p[5] = 0;
  p[6] = 0;
  p[7] = 0; /* CHS end (unused) */
  p[8] = (uint8_t)(start_lba & 0xFF);
  p[9] = (uint8_t)((start_lba >> 8) & 0xFF);
  p[10] = (uint8_t)((start_lba >> 16) & 0xFF);
  p[11] = (uint8_t)((start_lba >> 24) & 0xFF);
  uint32_t count =
      (total_sectors > start_lba) ? (total_sectors - start_lba) : 0;
  p[12] = (uint8_t)(count & 0xFF);
  p[13] = (uint8_t)((count >> 8) & 0xFF);
  p[14] = (uint8_t)((count >> 16) & 0xFF);
  p[15] = (uint8_t)((count >> 24) & 0xFF);
  mbr[510] = 0x55;
  mbr[511] = 0xAA;
}

extern "C" void installer_main(uint32_t magic, uint32_t addr) {
  serial_init();
  serial("\n\n[INSTALLER] Starting Chrysalis OS Installer...\n");

  /* 1. Initialize Subsystems */
  kmalloc_init();

    serial("[INSTALLER] initializing ATA...\n");
    ata_init();
    /* Keep cache flush enabled for verified writes */
    ata_set_skip_cache_flush(0);

  /* 2. Format Target Disk (Primary Master) */
  serial("[INSTALLER] Preparing target disk...\n");
  ata_set_allow_mbr_write(1);

  // Quick & Dirty Partitioning:
  // 1. Get Capacity (using a fixed large size if identify fails or just assume
  // 128MB+ for qemu) Real ata_identify returns sectors. Let's try to just
  // assume a partition at 2048.

  // Create MBR - Skipped for now, relying on raw device or pre-existing
  // partition table for simple tests. uint8_t* mbr = (uint8_t*)kmalloc(512);
  // ... clear ...
  // ... write partition 1 entry at offset 446 ...
  //   type 0x0C (FAT32 LBA)
  //   start 2048
  //   size = 200000 (approx 100MB dummy) or we need real size.
  //   active = 0x80

  // Actually, let's skip MBR creation for now and just format the "loop" device
  // or assumed partition logic? OS uses partitions. If I overwrite MBR, I need
  // to be careful. For specific task: "Standalone Installer". I will replace
  // `disk_format_fat32(0)` with `fat32_format(0, 0, ...)` if I format FLOPPY
  // style? No, HDD. I will use `disk_format_fat32(0)` if I assume that function
  // DID the partitioning. Since `disk_format_fat32` is missing, I'll replace it
  // with `fat32_format`. I'll define `fat32_format` external.

  /* 2. Format Disk */
  serial("[INSTALLER] Formatting Setup...\n");
  // We will treat the whole disk as one FAT32 volume starting at sector 0
  // (Superfloppy) OR partition 1. Let's go with Partition 1 at LBA 2048 for
  // better compatibility.
  uint32_t start_lba = 2048;
  uint32_t total_sectors = disk_get_capacity();
  if (total_sectors == 0)
    total_sectors = 262144; // Default 128MB
  if (total_sectors <= start_lba + 4096) {
    serial("[INSTALLER] ERROR: Disk too small (%u sectors)\n", total_sectors);
    return;
  }

  serial("[INSTALLER] Creating FAT32 Filesystem on Partition 1 (LBA %d)...\n",
         start_lba);
  if (fat32_format(start_lba, total_sectors - start_lba, "CHRYSALIS") != 0) {
    serial("[INSTALLER] ERROR: Formatting failed!\n");
    return;
  }
  /* Validate VBR */
  uint8_t *vbr = (uint8_t *)kmalloc(512);
  if (vbr) {
    if (disk_read_sector(start_lba, vbr) != 0) {
      serial("[INSTALLER] ERROR: VBR read failed at LBA %d\n", start_lba);
      kfree(vbr);
      return;
    }
    uint16_t bps = *(uint16_t *)(vbr + 11);
    if (vbr[510] != 0x55 || vbr[511] != 0xAA || bps != 512) {
      serial("[INSTALLER] ERROR: VBR invalid (bps=%d sig=%x%x)\n", (int)bps,
             vbr[510], vbr[511]);
      serial("[INSTALLER] VBR dump: %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x %02x %02x\n",
             vbr[0], vbr[1], vbr[2], vbr[3], vbr[4], vbr[5], vbr[6], vbr[7],
             vbr[8], vbr[9], vbr[10], vbr[11], vbr[12], vbr[13], vbr[14],
             vbr[15]);
      kfree(vbr);
      return;
    }
    kfree(vbr);
  }
  serial("[INSTALLER] Format Complete. Mounting FAT32...\n");

  if (fat32_init(0, start_lba) != 0) {
    serial("[INSTALLER] mount failed at LBA %d, trying LBA 0\n", start_lba);
    if (fat32_init(0, 0) != 0) {
      serial("[INSTALLER] ERROR: Failed to mount FAT32.\n");
      return;
    }
    fat32_set_mounted(0, 'a');
  } else {
    fat32_set_mounted(start_lba, 'a');
  }

  /* Dump BPB layout for debug */
  uint8_t *bpb_dbg = (uint8_t *)kmalloc(512);
  if (bpb_dbg && disk_read_sector(start_lba, bpb_dbg) == 0) {
    struct fat_bpb *bpb = (struct fat_bpb *)bpb_dbg;
    uint32_t fat_start = start_lba + bpb->reserved_sectors;
    uint32_t data_start =
        fat_start + (bpb->fats_count * bpb->sectors_per_fat_32);
    serial("[INSTALLER] BPB: total_sectors=%u reserved=%u fats=%u spf=%u spc=%u\n",
           bpb->total_sectors_32, bpb->reserved_sectors, bpb->fats_count,
           bpb->sectors_per_fat_32, bpb->sectors_per_cluster);
    serial("[INSTALLER] BPB: fat_start=%u data_start=%u\n",
           fat_start, data_start);
    kfree(bpb_dbg);
  } else if (bpb_dbg) {
    kfree(bpb_dbg);
  }

  /* 3. Create Directory Structure */
  serial("[INSTALLER] Creating directories...\n");
  char boot_path[6] = {'/', 'b', 'o', 'o', 't', 0};
  char chrys_path[16] = {'/', 'b', 'o', 'o', 't', '/', 'c', 'h',
                         'r', 'y', 's', 'a', 'l', 'i', 's', 0};
  char grub_path[11] = {'/', 'b', 'o', 'o', 't', '/', 'g', 'r', 'u', 'b', 0};
  char system_path[8] = {'/', 's', 'y', 's', 't', 'e', 'm', 0};
  char icons_dir[14] = {'/', 's', 'y', 's', 't', 'e', 'm', '/', 'i', 'c', 'o',
                        'n', 's', 0};

  int mr = fat32_create_directory_verified(boot_path, 1);
  if (mr != 0) {
    serial("[INSTALLER] ERROR: mkdir /boot failed (err=%d)\n", mr);
    return;
  }
  mr = fat32_create_directory_verified(chrys_path, 1);
  if (mr != 0) {
    serial("[INSTALLER] ERROR: mkdir /boot/chrysalis failed (err=%d)\n", mr);
    return;
  }
  mr = fat32_create_directory_verified(grub_path, 1);
  if (mr != 0) {
    serial("[INSTALLER] ERROR: mkdir /boot/grub failed (err=%d)\n", mr);
    return;
  }
  mr = fat32_create_directory_verified(system_path, 1);
  if (mr != 0) {
    serial("[INSTALLER] WARN: mkdir /system failed (err=%d), continuing\n", mr);
  }
  mr = fat32_create_directory_verified(icons_dir, 1);
  if (mr != 0) {
    serial("[INSTALLER] WARN: mkdir /system/icons failed (err=%d), continuing\n", mr);
  }

  /* Directory listings disabled to reduce stack usage and avoid instability */

  /* 4. Locate Source Files (Multiboot Modules) */
  void *kernel_data = NULL;
  size_t kernel_size = 0;
  void *boot_img = NULL;
  size_t boot_img_size = 0;
  void *core_img = NULL;
  size_t core_img_size = 0;
  void *icon_data[ICON_COUNT] = {0};
  size_t icon_sizes[ICON_COUNT] = {0};

  /* Scan multidoob tags (parsed manually here as we need raw addresses) */
  struct multiboot2_tag *tag = (struct multiboot2_tag *)(addr + 8);
  while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
    if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
      struct multiboot2_tag_module *mod = (struct multiboot2_tag_module *)tag;
      const char *cmdline = mod->string; /* Correct field name: char string[] */

      serial("[INSTALLER] Found Module: '%s' start=%x end=%x size=%d\n",
             cmdline, mod->mod_start, mod->mod_end,
             (int)(mod->mod_end - mod->mod_start));

      if (cmdline) {
        size_t cmd_len = 0;
        if (tag->size > sizeof(struct multiboot2_tag_module)) {
          cmd_len = tag->size - sizeof(struct multiboot2_tag_module);
        }
        if (cmd_len > 0 && cmdline[cmd_len - 1] == '\0') {
          cmd_len--;
        }
        char mod_name[64];
        normalize_module_name_len(cmdline, cmd_len, mod_name, sizeof(mod_name));
        serial("[INSTALLER] Module name parsed: '%s'\n", mod_name);

        int m_kernel = strcmp(mod_name, "kernel.bin");
        int m_boot = strcmp(mod_name, "boot.img");
        int m_core = strcmp(mod_name, "core.img");
        serial("[INSTALLER] mod_name cmp: kernel=%d boot=%d core=%d\n",
               m_kernel, m_boot, m_core);

        if (m_kernel == 0) {
          kernel_data = (void *)mod->mod_start;
          kernel_size = mod->mod_end - mod->mod_start;
          serial("[INSTALLER] Assigned to kernel.bin\n");
        } else if (m_boot == 0) {
          boot_img = (void *)mod->mod_start;
          boot_img_size = mod->mod_end - mod->mod_start;
          serial("[INSTALLER] Assigned to boot.img\n");
        } else if (m_core == 0) {
          core_img = (void *)mod->mod_start;
          core_img_size = mod->mod_end - mod->mod_start;
          serial("[INSTALLER] Assigned to core.img\n");
        } else {
          /* Try BMP icons */
          for (int i = 0; i < ICON_COUNT; i++) {
            if (strcmp(mod_name, g_icon_files[i]) == 0) {
              icon_data[i] = (void *)mod->mod_start;
              icon_sizes[i] = mod->mod_end - mod->mod_start;
              serial("[INSTALLER] Assigned to %s\n", g_icon_files[i]);
              goto mod_done;
            }
          }
          serial("[INSTALLER] Module '%s' did not match any expected file.\n",
                 cmdline);
        }
mod_done:
        ;
      }
    }
    tag = (struct multiboot2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
  }

  /* 4.1 Install GRUB boot code + core.img */
  serial("[INSTALLER] GRUB assets: boot_img=%x size=%d core_img=%x size=%d\n",
         (uint32_t)boot_img, (int)boot_img_size, (uint32_t)core_img,
         (int)core_img_size);
  if (!boot_img || boot_img_size < 446 || !core_img || core_img_size == 0) {
    serial(
        "[INSTALLER] ERROR: GRUB boot/core images missing in installer ISO.\n");
    return;
  }

  uint32_t core_sectors = (core_img_size + 511) / 512;
  if (1 + core_sectors >= start_lba) {
    serial("[INSTALLER] ERROR: core.img too large (%u sectors)\n",
           core_sectors);
    return;
  }

  uint8_t *mbr = (uint8_t *)kmalloc(512);
  if (!mbr) {
    serial("[INSTALLER] ERROR: no memory for MBR\n");
    return;
  }
  build_mbr(mbr, (const uint8_t *)boot_img, start_lba, total_sectors);
  disk_write_sector(0, mbr);
  kfree(mbr);

  serial("[INSTALLER] Writing GRUB core.img (%d bytes)...\n",
         (int)core_img_size);
  write_sectors(1, core_img, (uint32_t)core_img_size);

  /* 5. Write GRUB config early */
  const char *grub_cfg = "set timeout=3\n"
                         "set default=0\n"
                         "menuentry \"Chrysalis OS\" {\n"
                         "  multiboot2 /boot/chrysalis/kernel.bin\n"
                         "  boot\n"
                         "}\n";
  serial("[INSTALLER] Writing GRUB configuration...\n");
  fat32_create_file_verified("/boot/grub/grub.cfg", grub_cfg, strlen(grub_cfg),
                             1);
  fat32_create_file_verified("/grub.cfg", grub_cfg, strlen(grub_cfg), 1);

  /* Install core.img into /boot/grub for normal GRUB stage2 lookup */
  serial("[INSTALLER] Copying core.img to /boot/grub/core.img...\n");
  fat32_create_file_verified("/boot/grub/core.img", core_img,
                             (uint32_t)core_img_size, 1);

  /* 6. Install Kernel (chunked) */
  if (kernel_data && kernel_size > 0) {
    serial("[INSTALLER] Installing Kernel (%d bytes)...\n", (int)kernel_size);
    int r = fat32_create_file_alloc("/boot/chrysalis/kernel.bin", kernel_size);
    if (r != 0) {
      serial("[INSTALLER] ERROR: Failed to allocate kernel.bin (err=%d)\n", r);
      return;
    }
    const uint8_t *kp = (const uint8_t *)kernel_data;
    uint32_t chunk = 64 * 1024;
    uint32_t offset = 0;
    while (offset < kernel_size) {
      uint32_t n = kernel_size - offset;
      if (n > chunk) n = chunk;
      int wr = fat32_write_file_offset("/boot/chrysalis/kernel.bin", kp + offset, n, offset, 0);
      if (wr != 0) {
        serial("[INSTALLER] ERROR: kernel.bin chunk write failed (off=%d err=%d)\n", (int)offset, wr);
        return;
      }
      offset += n;
      serial("[INSTALLER] kernel.bin progress %d/%d\n", (int)offset, (int)kernel_size);
    }
    serial("[INSTALLER] Kernel Installed OK.\n");
    int32_t ksz = fat32_get_file_size("/boot/chrysalis/kernel.bin");
    serial("[INSTALLER] kernel.bin size on disk: %d\n", ksz);
    if (ksz != (int32_t)kernel_size) {
      serial("[INSTALLER] ERROR: kernel.bin size mismatch\n");
      return;
    }
  } else {
    serial("[INSTALLER] ERROR: Kernel module not found in memory!\n");
    return;
  }

  /* 7. Install Icons (BMP files) */
  serial("[INSTALLER] Installing Icons (BMP)...\n");
  int icons_written = 0;
  for (int i = 0; i < ICON_COUNT; i++) {
    if (!icon_data[i] || icon_sizes[i] == 0) continue;
    char path[48];
    /* "/system/icons/<name>" */
    path[0] = '/'; path[1] = 's'; path[2] = 'y'; path[3] = 's';
    path[4] = 't'; path[5] = 'e'; path[6] = 'm'; path[7] = '/';
    path[8] = 'i'; path[9] = 'c'; path[10] = 'o'; path[11] = 'n';
    path[12] = 's'; path[13] = '/';
    const char* nm = g_icon_files[i];
    int j = 0;
    while (nm[j] && (14 + j) < (int)sizeof(path) - 1) {
      path[14 + j] = nm[j];
      j++;
    }
    path[14 + j] = 0;

    serial("[INSTALLER] icon %s (%d bytes) -> %s\n",
           nm, (int)icon_sizes[i], path);
    int r = fat32_create_file_verified(path, icon_data[i], (uint32_t)icon_sizes[i], 0);
    if (r != 0) {
      serial("[INSTALLER] ERROR: icon write failed (%s err=%d)\n", nm, r);
      return;
    }
    icons_written++;
    kmalloc_reset();
  }
  serial("[INSTALLER] Icons Installed OK (%d files).\n", icons_written);

  /* /boot/grub listing disabled to reduce stack usage */

  int32_t gsz = fat32_get_file_size("/boot/grub/grub.cfg");
  serial("[INSTALLER] grub.cfg size: %d\n", gsz);

  fat_file_info_t c_entries[16];
  int c_count = fat32_read_directory("/boot/chrysalis", c_entries, 16);
  serial("[INSTALLER] /boot/chrysalis entries: %d\n", c_count);
  for (int i = 0; i < c_count; i++) {
    serial("  %s%s (%u)\n", c_entries[i].name, c_entries[i].is_dir ? "/" : "",
           c_entries[i].size);
  }

  serial("\n[INSTALLER] Installation Complete. Rebooting in 5s...\n");
  // Busy wait
  for (volatile int i = 0; i < 50000000; i++)
    ;

  outb(0x64, 0xFE); // Reboot
  while (1)
    asm volatile("hlt");
}

// String functions provided by string.cpp
