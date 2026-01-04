#include "elf.h"
#include <stdint.h>
#include <stddef.h>

extern void* kmalloc(uint32_t size);
extern void kfree(void* ptr);
extern void* memcpy(void* dst, const void* src, uint32_t n);
extern void* memset(void* dst, int c, uint32_t n);
extern void terminal_printf(const char* fmt, ...);

/* Basic bounds-safe accessor helpers */
static int safe_read_u16(const void* base, uint32_t size, uint32_t off, uint16_t* out) {
    if (off + sizeof(uint16_t) > size) return -1;
    const uint8_t* b = (const uint8_t*)base + off;
    *out = (uint16_t)(b[0] | (b[1] << 8));
    return 0;
}
static int safe_read_u32(const void* base, uint32_t size, uint32_t off, uint32_t* out) {
    if (off + sizeof(uint32_t) > size) return -1;
    const uint8_t* b = (const uint8_t*)base + off;
    *out = (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    return 0;
}

/* Quick magic + class + machine check */
int elf_is_valid(const void* data, uint32_t size) {
    if (!data || size < sizeof(elf32_ehdr_t)) return 0;
    const unsigned char* id = (const unsigned char*)data;
    if (id[0] != 0x7f || id[1] != 'E' || id[2] != 'L' || id[3] != 'F') return 0;
    if (id[4] != ELFCLASS32) return 0; /* 32-bit */
    if (id[5] != ELFDATA2LSB) return 0; /* little endian */
    /* we don't enforce version here */
    /* extra checks will be in elf_parse */
    return 1;
}

int elf_parse(const void* data, uint32_t size, elf_load_info_t* out) {
    if (!elf_is_valid(data, size)) return -1;
    if (!out) return -2;

    if (size < sizeof(elf32_ehdr_t)) return -3;
    const elf32_ehdr_t* eh = (const elf32_ehdr_t*)data;
    /* copy hdr */
    out->ehdr = *eh;
    out->entry_point = eh->e_entry;
    out->seg_count = 0;

    /* validate machine & type */
    if (eh->e_machine != EM_386) {
        terminal_printf("[elf] unsupported machine: %u\n", eh->e_machine);
        return -4;
    }
    if (eh->e_type != ET_EXEC && eh->e_type != 1 /* ET_REL optional */) {
        terminal_printf("[elf] unsupported type: %u\n", eh->e_type);
        /* we'll still try to parse for ET_EXEC primarily */
    }

    /* iterate program headers */
    uint32_t phoff = eh->e_phoff;
    uint16_t phentsize = eh->e_phentsize;
    uint16_t phnum = eh->e_phnum;

    if (!phentsize || !phnum) return -5;

    for (uint16_t i = 0; i < phnum; ++i) {
        uint32_t entry_off = phoff + (uint32_t)i * (uint32_t)phentsize;
        if (entry_off + sizeof(elf32_phdr_t) > size) return -6;
        const elf32_phdr_t* ph = (const elf32_phdr_t*)((const uint8_t*)data + entry_off);
        if (ph->p_type == PT_LOAD) {
            if (out->seg_count >= ELF_MAX_LOAD_SEGMENTS) {
                terminal_printf("[elf] too many load segments (> %d)\n", ELF_MAX_LOAD_SEGMENTS);
                return -7;
            }
            elf_segment_t* seg = &out->segments[out->seg_count++];
            seg->kernel_buf = NULL;
            seg->vaddr = ph->p_vaddr;
            seg->filesz = ph->p_filesz;
            seg->memsz = ph->p_memsz;
            seg->flags = ph->p_flags;
            /* bounds check the file region */
            if (ph->p_offset + ph->p_filesz > size) {
                terminal_printf("[elf] program header filesz out of range\n");
                return -8;
            }
        }
    }

    return 0;
}

int elf_load_into_kernel_space(const void* data, uint32_t size, elf_load_info_t* out) {
    if (!data || !out) return -1;
    int r = elf_parse(data, size, out);
    if (r < 0) return r;

    /* allocate kernel buffers and copy filesz, zero the bss up to memsz */
    for (int i = 0; i < out->seg_count; ++i) {
        elf_segment_t* s = &out->segments[i];
        uint32_t need = s->filesz;
        /* allocate at least memsz for convenience? We'll allocate filesz only and remember memsz for page-zeroing later */
        void* buf = kmalloc( (need > 0) ? need : 1 );
        if (!buf) {
            terminal_printf("[elf] kmalloc failed for segment %d\n", i);
            /* unwind */
            for (int j = 0; j < i; ++j) {
                if (out->segments[j].kernel_buf) kfree(out->segments[j].kernel_buf);
                out->segments[j].kernel_buf = NULL;
            }
            return -2;
        }
        /* copy content from file into kernel buffer */
        if (s->filesz > 0) {
            const void* src = (const uint8_t*)data + /* p_offset: must re-read phdr to know offset */ 0;
            /* We didn't store p_offset in elf_segment_t to keep header minimal.
               To avoid re-parsing offsets here, we will re-parse program headers
               directly from header data. Simpler approach: re-walk phdrs to copy. */
            /* --- fallback: do a re-walk to find corresponding p_offset --- */
            const elf32_ehdr_t* eh = (const elf32_ehdr_t*)data;
            uint32_t phoff = eh->e_phoff;
            uint16_t phentsize = eh->e_phentsize;
            uint16_t phnum = eh->e_phnum;
            uint32_t found_offset = 0;
            int matched = 0;
            for (uint16_t p = 0; p < phnum; ++p) {
                uint32_t off = phoff + (uint32_t)p * (uint32_t)phentsize;
                const elf32_phdr_t* ph = (const elf32_phdr_t*)((const uint8_t*)data + off);
                if (ph->p_type == PT_LOAD && ph->p_vaddr == s->vaddr && ph->p_filesz == s->filesz && ph->p_memsz == s->memsz) {
                    found_offset = ph->p_offset;
                    matched = 1;
                    break;
                }
            }
            if (!matched) {
                terminal_printf("[elf] failed to locate original p_offset for segment (vaddr=0x%x)\n", s->vaddr);
                kfree(buf);
                /* unwind */
                for (int j = 0; j < i; ++j) {
                    if (out->segments[j].kernel_buf) kfree(out->segments[j].kernel_buf);
                    out->segments[j].kernel_buf = NULL;
                }
                return -3;
            }
            const void* srcptr = (const uint8_t*)data + found_offset;
            memcpy(buf, srcptr, s->filesz);
        } else {
            /* nothing to copy */
            memset(buf, 0, 1);
        }
        s->kernel_buf = buf;
        /* NOTE: BSS zeroing is performed later when mapping into process address space (map memsz and zero remaining) */
    }

    return 0;
}

void elf_unload_kernel_space(elf_load_info_t* info) {
    if (!info) return;
    for (int i = 0; i < info->seg_count; ++i) {
        if (info->segments[i].kernel_buf) {
            kfree(info->segments[i].kernel_buf);
            info->segments[i].kernel_buf = NULL;
        }
    }
    info->seg_count = 0;
}

/* convenience */
int elf_load_from_buffer(void* buf, uint32_t len, elf_load_info_t* out) {
    return elf_load_into_kernel_space(buf, len, out);
}
