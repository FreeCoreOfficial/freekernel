void paging_init_kernel(void);
void* create_user_pagetable(); /* return pointer la page directory fizic */
void map_user_page(void* pagedir, void* vaddr, void* phys, uint32_t flags);
