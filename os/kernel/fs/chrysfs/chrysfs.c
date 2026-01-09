#include "chrysfs.h"
#include "../../string.h"
#include "../../mem/kmalloc.h"

extern void serial(const char *fmt, ...);
extern void terminal_writestring(const char *s);
extern void terminal_printf(const char *fmt, ...);

#define CHRYSFS_MAGIC 0x43485259 // "CHRY"
#define BLOCK_SIZE 512

/* Layout:
 * LBA 0: Superblock
 * LBA 1: Block Bitmap (covers 512*8 = 4096 blocks = 2MB)
 * LBA 2..65: Inodes (64 inodes, 1 sector each)
 * LBA 66+: Data Blocks
 */
#define LBA_SUPERBLOCK 0
#define LBA_BITMAP     1
#define LBA_INODES     2
#define MAX_INODES     64
#define LBA_DATA       (LBA_INODES + MAX_INODES)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t num_inodes;
    uint32_t data_start;
    uint8_t  padding[492];
} __attribute__((packed)) chrysfs_superblock_t;

typedef struct {
    uint32_t magic;      // 0xCAFEBABE if valid
    uint32_t type;       // 1=file
    uint32_t size;
    char     name[64];   // Filename
    uint32_t blocks[100]; // Direct pointers to LBAs
    uint8_t  padding[36]; // Pad to 512
} __attribute__((packed)) chrysfs_inode_t;

#define INODE_MAGIC 0xCAFEBABE

static block_device_t *mounted_dev = 0;
static uint32_t fs_data_start = LBA_DATA;

void chrysfs_init(void) {
    serial("[FS] CHRYS_FS driver initialized\n");
}

/* Helper: Get filename from path */
static const char* get_filename(const char* path) {
    const char* p = path;
    const char* last_slash = 0;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    if (last_slash) return last_slash + 1;
    return path;
}

/* Helper: Bitmap operations */
static int alloc_block(block_device_t *dev) {
    uint8_t *bmp = (uint8_t*)kmalloc(BLOCK_SIZE);
    if (!bmp) return 0;
    
    dev->read(dev, LBA_BITMAP, 1, bmp);
    
    for (int i = 0; i < BLOCK_SIZE * 8; i++) {
        if (!((bmp[i/8] >> (i%8)) & 1)) {
            // Found free
            bmp[i/8] |= (1 << (i%8));
            dev->write(dev, LBA_BITMAP, 1, bmp);
            kfree(bmp);
            return fs_data_start + i;
        }
    }
    
    kfree(bmp);
    return 0; // Full
}

/* Helper: Find inode by name */
static int find_inode(block_device_t *dev, const char* name, chrysfs_inode_t* out_inode, uint32_t* out_lba) {
    chrysfs_inode_t *node = (chrysfs_inode_t*)kmalloc(BLOCK_SIZE);
    if (!node) return -1;

    for (int i = 0; i < MAX_INODES; i++) {
        uint32_t lba = LBA_INODES + i;
        dev->read(dev, lba, 1, (uint8_t*)node);
        
        if (node->magic == INODE_MAGIC && strcmp(node->name, name) == 0) {
            if (out_inode) memcpy(out_inode, node, sizeof(chrysfs_inode_t));
            if (out_lba) *out_lba = lba;
            kfree(node);
            return 0; // Found
        }
    }
    
    kfree(node);
    return -1;
}

int chrysfs_format(block_device_t *dev) {
    serial("[FS] Formatting %s with CHRYS_FS...\n", dev->name);
    
    uint8_t *buf = (uint8_t*)kmalloc(BLOCK_SIZE);
    if (!buf) return -1;
    memset(buf, 0, BLOCK_SIZE);

    // 1. Write Superblock
    chrysfs_superblock_t *sb = (chrysfs_superblock_t*)buf;
    sb->magic = CHRYSFS_MAGIC;
    sb->version = 1;
    sb->block_size = BLOCK_SIZE;
    sb->num_inodes = MAX_INODES;
    sb->data_start = LBA_DATA;
    dev->write(dev, LBA_SUPERBLOCK, 1, buf);

    // 2. Clear Bitmap
    memset(buf, 0, BLOCK_SIZE);
    dev->write(dev, LBA_BITMAP, 1, buf);

    // 3. Clear Inodes
    for (int i = 0; i < MAX_INODES; i++) {
        dev->write(dev, LBA_INODES + i, 1, buf);
    }

    serial("[FS] Format complete.\n");
    kfree(buf);
    return 0;
}

int chrysfs_mount(block_device_t *dev, const char *mountpoint) {
    if (!dev) return -1;
    
    uint8_t *buf = (uint8_t*)kmalloc(BLOCK_SIZE);
    if (!buf) return -1;
    
    dev->read(dev, LBA_SUPERBLOCK, 1, buf);
    
    chrysfs_superblock_t *sb = (chrysfs_superblock_t*)buf;
    if (sb->magic != CHRYSFS_MAGIC) {
        serial("[FS] Invalid magic on %s. Mount failed (not ChrysFS).\n", dev->name);
        kfree(buf);
        return -1;
    }
    
    fs_data_start = sb->data_start;
    mounted_dev = dev;
    
    serial("[FS] Mounted CHRYS_FS from %s at %s\n", dev->name, mountpoint);
    kfree(buf);
    return 0;
}

int chrysfs_ls(const char *path) {
    if (!mounted_dev) return -1;
    (void)path; // Flat FS, ignore path for now

    chrysfs_inode_t *node = (chrysfs_inode_t*)kmalloc(BLOCK_SIZE);
    if (!node) return -1;

    terminal_printf("Listing files on %s:\n", mounted_dev->name);
    int count = 0;
    for (int i = 0; i < MAX_INODES; i++) {
        mounted_dev->read(mounted_dev, LBA_INODES + i, 1, (uint8_t*)node);
        if (node->magic == INODE_MAGIC) {
            terminal_printf("  [FILE] %s (%u bytes)\n", node->name, node->size);
            count++;
        }
    }
    if (count == 0) terminal_writestring("  (empty)\n");
    
    kfree(node);
    return 0;
}

int chrysfs_create_file(const char *path, const void *data, uint32_t size) {
    if (!mounted_dev) return -1;
    
    const char* fname = get_filename(path);
    
    // Check if exists (overwrite not supported in this simple version)
    if (find_inode(mounted_dev, fname, 0, 0) == 0) {
        serial("[FS] File %s already exists.\n", fname);
        return -1;
    }

    // Find free inode
    chrysfs_inode_t *node = (chrysfs_inode_t*)kmalloc(BLOCK_SIZE);
    if (!node) return -1;
    
    int inode_lba = -1;
    for (int i = 0; i < MAX_INODES; i++) {
        mounted_dev->read(mounted_dev, LBA_INODES + i, 1, (uint8_t*)node);
        if (node->magic != INODE_MAGIC) {
            inode_lba = LBA_INODES + i;
            break;
        }
    }
    
    if (inode_lba == -1) {
        serial("[FS] No free inodes.\n");
        kfree(node);
        return -1;
    }

    // Setup inode
    memset(node, 0, sizeof(chrysfs_inode_t));
    node->magic = INODE_MAGIC;
    node->type = 1;
    node->size = size;
    strncpy(node->name, fname, 63);

    // Allocate blocks and write data
    uint32_t bytes_written = 0;
    int block_idx = 0;
    const uint8_t* ptr = (const uint8_t*)data;

    while (bytes_written < size && block_idx < 100) {
        uint32_t blk = alloc_block(mounted_dev);
        if (blk == 0) {
            serial("[FS] Disk full.\n");
            break;
        }
        
        node->blocks[block_idx++] = blk;
        
        uint32_t chunk = size - bytes_written;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        
        // Write partial block? We write full sector, padding with 0 if needed
        uint8_t* sector = (uint8_t*)kmalloc(BLOCK_SIZE);
        memset(sector, 0, BLOCK_SIZE);
        memcpy(sector, ptr + bytes_written, chunk);
        mounted_dev->write(mounted_dev, blk, 1, sector);
        kfree(sector);
        
        bytes_written += chunk;
    }

    // Save inode
    mounted_dev->write(mounted_dev, inode_lba, 1, (uint8_t*)node);
    
    kfree(node);
    serial("[FS] Created file %s (%u bytes)\n", fname, bytes_written);
    return 0;
}

int chrysfs_read_file(const char *path, void *buf, uint32_t max_size) {
    if (!mounted_dev) return -1;
    
    const char* fname = get_filename(path);
    
    chrysfs_inode_t *node = (chrysfs_inode_t*)kmalloc(BLOCK_SIZE);
    if (!node) return -1;
    
    if (find_inode(mounted_dev, fname, node, 0) != 0) {
        kfree(node);
        return -1;
    }
    
    uint32_t to_read = node->size;
    if (to_read > max_size) to_read = max_size;
    
    uint32_t bytes_read = 0;
    int block_idx = 0;
    uint8_t* out = (uint8_t*)buf;
    
    uint8_t* sector = (uint8_t*)kmalloc(BLOCK_SIZE);
    
    while (bytes_read < to_read && block_idx < 100) {
        uint32_t blk = node->blocks[block_idx++];
        if (blk == 0) break;
        
        mounted_dev->read(mounted_dev, blk, 1, sector);
        
        uint32_t chunk = to_read - bytes_read;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        
        memcpy(out + bytes_read, sector, chunk);
        bytes_read += chunk;
    }
    
    kfree(sector);
    kfree(node);
    return bytes_read;
}