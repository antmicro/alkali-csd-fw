#include <cstdio>
#include <cstring>
#include <cstdint>
#include <linux/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XLNK_PATH "/dev/xlnk"
#define XLNK_MAGIC 'X'

#define ALLOC_IOCTL _IOWR(XLNK_MAGIC,   2, unsigned long)
#define  FREE_IOCTL _IOWR(XLNK_MAGIC,   3, unsigned long)
#define RESET_IOCTL _IOWR(XLNK_MAGIC, 101, unsigned long)

typedef uint64_t xlnk_intptr_type;
typedef int32_t xlnk_int_type;
typedef uint32_t xlnk_uint_type;
typedef uint8_t xlnk_byte_type;
typedef int8_t xlnk_char_type;

union xlnk_args {
        struct __attribute__ ((__packed__)) {
                xlnk_uint_type len;
                xlnk_int_type id;
                xlnk_intptr_type phyaddr;
                xlnk_byte_type cacheable;
        } allocbuf;
        struct __attribute__ ((__packed__)) {
                xlnk_uint_type id;
                xlnk_intptr_type buf;
        } freebuf;
};

typedef struct cma_mem {
	uint64_t phys;
	void    *virt;
	uint32_t len;
	int32_t  id;
} cma_mem_t;

/* Functional prototpes from xlnk */

unsigned long xlnkGetBufPhyAddr(void*);

static int xlnkfd;

std::vector<cma_mem_t*> maps;

void cma_init(void) {
    xlnkfd = open(XLNK_PATH, O_RDWR | O_CLOEXEC);
    if (xlnkfd < 0) {
        printf("Reset failed - could not open device: %s(%d)\n", XLNK_PATH, xlnkfd);
        return;
    }
    if (ioctl(xlnkfd, RESET_IOCTL, 0) < 0) {
        printf("Reset failed - IOCTL failed: %d\n", errno);
    }
    printf("Succesfully opened XLNK device (%s, %d)\n", XLNK_PATH, xlnkfd);
}

void cma_clean(void) {
    close(xlnkfd);
}

static void *cma_mmap(int32_t id, uint32_t len) {
	return mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC | PROT_NONE, MAP_SHARED, xlnkfd, (id << 4) * 4096);
}

/* CMA implementations */
void *cma_alloc(uint32_t len, uint32_t cacheable) {
    union xlnk_args arg = {0};
    arg.allocbuf.len = len;
    arg.allocbuf.cacheable = cacheable ? 1 : 0;

    if (ioctl(xlnkfd, ALLOC_IOCTL, &arg) < 0) {
        printf("Alloc failed - IOCTL failed: %d\n", errno);
	return NULL;
    }

    cma_mem_t *mem = new cma_mem_t;

    mem->phys = arg.allocbuf.phyaddr;
    mem->len = len;
    mem->id = arg.allocbuf.id;
    mem->virt = cma_mmap(mem->id, mem->len);

    maps.push_back(mem);

    return mem->virt;
}

unsigned long cma_get_phy_addr(void *buf) {
    for(auto const& val: maps) {
        if(val->virt == buf)
            return val->phys;
    }
    return 0;
}

void cma_free(void *buf) {
    auto it = maps.begin();
    while(it != maps.end()) {
        if((*it)->virt == buf) {
            union xlnk_args arg = {0};
            arg.freebuf.id = (*it)->id;

            munmap((*it)->virt, (*it)->len);

            if (ioctl(xlnkfd, FREE_IOCTL, &arg) < 0) {
                printf("Free failed - IOCTL failed: %d\n", errno);
            }

            maps.erase(it);

            return;
        } else {
            it++;
        }
    }
}

void cma_flush_cache(void* buf, unsigned int phys_addr, int size) {
    uintptr_t begin = (uintptr_t)buf;
    uintptr_t line = begin &~0x3FULL;
    uintptr_t end = begin + size;

    uintptr_t stride = 64; // TODO: Make this architecture dependent

    asm volatile(
    "loop:\n\t"
    "dc civac, %[line]\n\t"
    "add %[line], %[line], %[stride]\n\t"
    "cmp %[line], %[end]\n\t"
    "b.lt loop\n\t"
    ::[line] "r" (line),
    [stride] "r" (stride),
    [end] "r" (end)
    : "cc", "memory"
    );
}

void cma_invalidate_cache(void* buf, unsigned int phys_addr, int size) {
    cma_flush_cache(buf, phys_addr, size);
}

#ifdef __cplusplus
}
#endif
