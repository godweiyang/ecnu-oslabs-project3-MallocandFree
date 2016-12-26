#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mem.h"

int m_error;
int have_init = 0;

typedef struct mem_control_block {
    int is_available;
    int size;
}MCB;

void *region_start;
void *region_end;

void mem_dump() {
	void *current_address = region_start;
    MCB *current_mcb;
    while (current_address != region_end) {
		current_mcb = (MCB *)current_address;
		if (current_mcb->is_available) {
			printf("free mcb: %p ~ %p\n\n", current_address, current_address + current_mcb->size);
		}
		current_address += current_mcb->size;
	}
}

int mem_init(int size_of_region) {
    if (size_of_region <= 0 || have_init == 1) {
        m_error = E_BAD_ARGS;
        return -1;
    }
    have_init = 1;
    int page_size = getpagesize();
    size_of_region = ((size_of_region + sizeof(MCB) - 1) / page_size) * page_size + page_size;
    int fd = open("/dev/zero", O_RDWR);
    region_start = mmap(NULL, size_of_region, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    if (region_start == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    MCB *current_mcb = (MCB *)region_start;
    current_mcb->is_available = 1;
    current_mcb->size = size_of_region;
    region_end = region_start + size_of_region;
    //mem_dump();
    return 0;
}

void *mem_alloc(int size, int style) {
	size = size + sizeof(MCB);
    size = (((size - 1) >> 3) << 3) + 8;
    void *current_address = region_start;
    MCB *current_mcb;
    void *alloc_address = NULL;
    if (style == M_FIRSTFIT) {
    	while (current_address != region_end) {
		    current_mcb = (MCB *)current_address;
		    if (current_mcb->is_available && current_mcb->size >= size) {
				alloc_address = current_address;
				break;
			}
		    current_address += current_mcb->size;
		}
    } else if (style == M_BESTFIT) {
    	while (current_address != region_end) {
		    current_mcb = (MCB *)current_address;
		    if (current_mcb->is_available && current_mcb->size >= size
		    	&& (alloc_address == NULL || current_mcb->size < ((MCB *)alloc_address)->size)) {
				alloc_address = current_address;
			}
		    current_address += current_mcb->size;
		}
    } else {
    	while (current_address != region_end) {
		    current_mcb = (MCB *)current_address;
		    if (current_mcb->is_available && current_mcb->size >= size
		    	&& (alloc_address == NULL || current_mcb->size > ((MCB *)alloc_address)->size)) {
				alloc_address = current_address;
			}
		    current_address += current_mcb->size;
		}
    }
    if (alloc_address == NULL) {
        m_error = E_NO_SPACE;
        return NULL;
    }
    MCB *alloc_mcb = (MCB *)alloc_address;
    if (alloc_mcb->size > size) {
    	MCB *next_mcb = (MCB *)(alloc_address + size);
    	next_mcb->is_available = 1;
    	next_mcb->size = alloc_mcb->size - size;
    }
    alloc_mcb->is_available = 0;
    alloc_mcb->size = size;
    alloc_address += sizeof(MCB);
    //mem_dump();
    return alloc_address;
}

int mem_free(void *ptr) {
	if (ptr == NULL) {
		return -1;
	}
	ptr = ptr - sizeof(MCB);
	MCB *current_mcb = (MCB *)ptr;
	current_mcb->is_available = 1;
	MCB *next_mcb = (MCB *)(ptr + current_mcb->size);
	if (ptr + current_mcb->size < region_end && next_mcb->is_available == 1) {
		current_mcb->size += next_mcb->size;
	}
	void *pre_address = region_start;
    MCB *pre_mcb;
    while (pre_address < ptr) {
    	pre_mcb = (MCB *)pre_address;
    	if (pre_address + pre_mcb->size == ptr && pre_mcb->is_available == 1) {
    		pre_mcb->size += current_mcb->size;
    	}
    	pre_address += pre_mcb->size;
    }
    //mem_dump();
    return 0;
}

