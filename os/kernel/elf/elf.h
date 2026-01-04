#pragma once

#include <stdint.h>

#define EI_NIDENT 16

/* minimal constants */
#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EV_CURRENT 1

#define ET_EXEC 2
#define EM_386 3

/* program header types */
#define PT_NULL 0
#define PT_LOAD 1

/* p_flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* ELF header (32-bit) */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_ehdr_t;

/* program header (32-bit) */
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

#define ELF_MAX_LOAD_SEGMENTS 8

typedef struct {
    void* kernel_buf; /* allocated kernel buffer containing the segment contents (filesz) */
    uint32_t vaddr;   /* preferred user virtual address from ELF */
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;   /* PF_* */
} elf_segment_t;

typedef struct {
    elf32_ehdr_t ehdr; /* copy of header (makes ownership simpler) */
    elf_segment_t segments[ELF_MAX_LOAD_SEGMENTS];
    int seg_count;
    uint32_t entry_point;
} elf_load_info_t;

/* API: simple return codes: 0 ok, <0 error */
int elf_is_valid(const void* data, uint32_t size);
int elf_parse(const void* data, uint32_t size, elf_load_info_t* out);
int elf_load_into_kernel_space(const void* data, uint32_t size, elf_load_info_t* out);
void elf_unload_kernel_space(elf_load_info_t* info);

/* convenience: read buffer and produce loaded info (parse + allocate copies) */
int elf_load_from_buffer(void* buf, uint32_t len, elf_load_info_t* out);
