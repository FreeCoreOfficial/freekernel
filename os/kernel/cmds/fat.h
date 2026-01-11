#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int cmd_fat(int argc, char **argv);

/* Încearcă să monteze automat prima partiție FAT32 găsită */
void fat_automount(void);

/* Listează conținutul unui director (path="/" pentru root) */
void fat32_list_directory(const char* path);

/* Structure for directory listing API */
typedef struct {
    char name[13];
    uint32_t size;
    uint8_t is_dir;
} fat_file_info_t;

/* Read directory entries into a buffer. Returns number of entries read. */
int fat32_read_directory(const char* path, fat_file_info_t* out, int max_entries);

/* Citește un fișier complet (limitat la max_size) */
int fat32_read_file(const char* path, void* buf, uint32_t max_size);

/* Citește dintr-un fișier de la un offset specificat (pentru fișiere mari) */
int fat32_read_file_offset(const char* path, void* buf, uint32_t size, uint32_t offset);

/* Creează un fișier (sau suprascrie) */
int fat32_create_file(const char* path, const void* data, uint32_t size);

/* Delete a file from the root directory */
int fat32_delete_file(const char* path);

/* Create a directory in the root directory */
int fat32_create_directory(const char* path);

/* Check if a directory exists */
int fat32_directory_exists(const char* path);

/* Format a partition with FAT32 */
int fat32_format(uint32_t lba, uint32_t sector_count, const char* label);

/* Get file size (returns -1 if not found) */
int32_t fat32_get_file_size(const char* path);

#ifdef __cplusplus
}
#endif
