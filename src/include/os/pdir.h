//
// Page directory routines
//
#ifndef PDIR_H
#define PDIR_H

typedef unsigned long pte_t;

#define PAGESIZE       4096
#define PAGESHIFT      12
#define PTES_PER_PAGE  (PAGESIZE / sizeof(pte_t))

#define PT_PRESENT   0x001
#define PT_WRITABLE  0x002
#define PT_USER      0x004
#define PT_ACCESSED  0x020
#define PT_DIRTY     0x040

#define PT_GUARD     0x200
#define PT_FILE      0x400

#define PT_USER_READ    (PT_PRESENT | PT_USER)
#define PT_USER_WRITE   (PT_PRESENT | PT_USER | PT_WRITABLE)

#define PT_FLAGMASK     (PT_PRESENT | PT_WRITABLE | PT_USER | PT_ACCESSED | PT_DIRTY | PT_GUARD | PT_FILE)
#define PT_PROTECTMASK  (PT_WRITABLE | PT_USER | PT_GUARD)

#define PT_PFNMASK   0xFFFFF000
#define PT_PFNSHIFT  12

#define PDEIDX(vaddr)  (((unsigned long) vaddr) >> 22)
#define PTEIDX(vaddr)  ((((unsigned long) vaddr) >> 12) & 0x3FF)
#define PGOFF(vaddr)   (((unsigned long) vaddr) & 0xFFF)
#define PTABIDX(vaddr) (((unsigned long) vaddr) >> 12)

#define PAGES(x) (((unsigned long)(x) + (PAGESIZE - 1)) >> PAGESHIFT)
#define PAGEADDR(x) ((unsigned long)(x) & ~(PAGESIZE - 1))

#define PTOB(x) ((unsigned long)(x) << PAGESHIFT)
#define BTOP(x) ((unsigned long)(x) >> PAGESHIFT)

#define SET_PDE(vaddr, val) set_page_dir_entry(&pdir[PDEIDX(vaddr)], (val))
#define SET_PTE(vaddr, val) set_page_table_entry(&ptab[PTABIDX(vaddr)], (val))
#define GET_PDE(vaddr) (pdir[PDEIDX(vaddr)])
#define GET_PTE(vaddr) (ptab[PTABIDX(vaddr)])

struct pdirstat {
    int present;
    int user;
    int kernel;
    int readonly;
    int readwrite;
    int accessed;
    int dirty;
};

#ifdef KERNEL

extern pte_t *pdir;
extern pte_t *ptab;

krnlapi void map_page(void *vaddr, unsigned long pfn, unsigned long flags);
krnlapi void unmap_page(void *vaddr);
krnlapi unsigned long virt2phys(void *vaddr);
krnlapi unsigned long virt2pfn(void *vaddr);
krnlapi pte_t get_page_flags(void *vaddr);
krnlapi void set_page_flags(void *vaddr, unsigned long flags);
krnlapi int page_mapped(void *vaddr);
krnlapi int page_directory_mapped(void *vaddr);
krnlapi void unguard_page(void *vaddr);
krnlapi void clear_dirty(void *vaddr);

krnlapi int mem_access(void *vaddr, int size, pte_t access);
krnlapi int str_access(char *s, pte_t access);

void init_pdir();
int pdir_proc(struct proc_file *pf, void *arg);
int virtmem_proc(struct proc_file *pf, void *arg);
int pdir_stat(void *addr, int len, struct pdirstat *buf);

#endif

#endif
