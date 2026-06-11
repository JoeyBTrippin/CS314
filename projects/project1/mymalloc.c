#include <stdio.h>
#include "mymalloc.h"

typedef struct BlockHeader {
	size_t size;
	int is_free;
	struct BlockHeader* next;
	struck BlockHeader* prev;
} BlockHeader;

#define HEADER_SIZE sizeof(BlockHeader)

static BlockHeader* fastbins[NUM_FASTBINS] = {NULL};
static BlockHeader* regbins[NUM_REGULAR_BINS] = {NULL};


// Find index of Fast Bin
static int index_fastbin (size_t size) {
	if (size == 16) return 0;
	if (size == 24) return 1;
	if (size == 32) return 2;
	if (size == 40) return 3;
	return -1;
}

// Find index of Regular Bin
static int index_regbin (size_t size) {
	if (size > 2048) return 3;
	if (size > 512) return 2;
	if (size > 128) return 1;
	if (size > 48) return 0;
	return -1;
}

// HELPER: remove 
static void rmove_from_bin (BlockHeader** bin, BlockHeader* block {

}

void* my_malloc (size_t size) {
	if (size == 0) return NULL;
	
	// FIND: size needed
	size_t total_size = (size + HEADER_SIZE + 7) & ~7;
	
	// FASTBIN path
	if (total <= 40) {
		int index = index_fastbin(total_size);

		if (fastbin[index]) { // bin is not empty
			BlackHeader* block = fastbin[index];
			fastbin[index] = blk->next;
			block->is_free = 0;
			return (char*)block + HEADER_SIZE;
		} // fall through to regbin
	}  

	// REGULARBIN path
	int index = index_regbin(total_size);
	BlockHeader* cur = regbins[index];

	while (cur && cur->size < total_size) // FIND: appropriate location
		cur = cur->next;
	
	if (cur) { // location not empty
		
	}
}


void my_free (void* ptr) {
	if (!ptr) return;

	BlockHeader* h = ((BlockHeader*)ptr) - 1;
	h->is_free = 1;

	if (h->size <= 40) {
		
	}
}

void dump_bin_statistics (void) {
	printf("Fastbins:\n");
	for (int i = 0; i < NUM_FASTBINS; i++) {
		int count = 0;
		size_t = total = 0;
		BlockHeader* cur = fastbins[i];
		while (cur) {
			count++;
			total += cur->size;
			cur = cur->next;
		}

	}
}

int main () {
	

	return 0;
}
