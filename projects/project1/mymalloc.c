#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mymalloc.h"

typedef struct BlockHeader {
	size_t size;
	int is_free;
	struct BlockHeader* next;
	struct BlockHeader* prev;
} BlockHeader;

#define HEADER_SIZE sizeof(BlockHeader)
#define PAGE 4096

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

// HELPER: remove block from bin
static void remove_from_bin (BlockHeader** bin, BlockHeader* block) {
	if (block->prev) // previous block not empty
		block->prev->next = block->next;
	else // previous block empty
		*bin = block->next;
	
	if (block->next) // next block not empty
		block->next->prev = block->prev;
	
	// Clear block
	block->next = block->prev = NULL;
}

static void insert_sorted (BlockHeader** bin, BlockHeader* block) {
	block->next = block->prev = NULL; // prevent frevious list members

	if (*bin == NULL) { // bin is empty
		*bin = block;
		return;
	}

	BlockHeader* cur = *bin;
	while (cur && (cur->size < block->size)) { // find appropriate location
		cur = cur->next;
	}

	if (cur == *bin) { // insert at head
		block->next = cur;
		cur->prev = block;
		*bin = block;
		return;
	}

	if (cur == NULL) { // insert at tail
		BlockHeader* tail = *bin;
		while (tail->next) tail = tail->next;
		tail->next = block;
		block->prev = tail;
		return;
	}
	
	// insert in middle
	block->next = cur;
	block->prev = cur->prev;
	cur->prev->next = block;
	cur->prev = block;
}

static BlockHeader* request_from_os (size_t size) {
	size_t total_size = size + HEADER_SIZE;
	size_t pages = (total_size + PAGE - 1) / PAGE;
	size_t alloc_size = pages * PAGE;

	void* region = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if (region == MAP_FAILED) // allocation failed 
		return NULL;

	BlockHeader* head = (BlockHeader*) region;
	head->size = alloc_size;
	head->is_free = 0;
	head->next = head->prev = NULL;

	size_t leftover = alloc_size - size - HEADER_SIZE;
	if (leftover >= 48) {
		BlockHeader* free_block = (BlockHeader*)((char*)region + HEADER_SIZE + size);
		free_block->size = leftover;
		free_block->is_free = 1;
		free_block->next = free_block->prev = NULL;

		int idx = index_regbin(leftover);
		insert_sorted(&regbins[idx], free_block);
	}

	return head;
}

void* my_malloc (size_t size) {
	if (size == 0) return NULL;
	
	// FIND: size needed
	size_t total_size = (size + HEADER_SIZE + 7) & ~7;
	
	// FAST BIN path
	if (total_size <= 40) {
		int index = index_fastbin(total_size);

		if (fastbins[index]) { // bin is not empty
			BlockHeader* block = fastbins[index];
			fastbins[index] = block->next;
			block->is_free = 0;
			return (char*)block + HEADER_SIZE;
		} // fall through to regbin
	}  

	// REGULAR BIN path
	int index = index_regbin(total_size);
	BlockHeader* cur = regbins[index];

	while (cur && cur->size < total_size) // FIND: appropriate location
		cur = cur->next;
	
	if (cur) { // location not empty
		remove_from_bin(&regbins[index], cur);
		cur->is_free = 0;

		size_t leftover = cur->size - total_size;
		if (leftover >= 48) {
			BlockHeader* split = (BlockHeader*)((char*)cur + total_size);
			split->size = leftover;
			split->is_free = 1;
			split->next = split->prev = NULL;

			int idx = index_regbin(leftover);
			insert_sorted(&regbins[idx], split);

			cur->size = total_size;

		}

		return (char*)cur + HEADER_SIZE;
	}

	// MMAP path: reguesting memory
	BlockHeader* block = request_from_os(total_size);
	return (char*)block + HEADER_SIZE;


}


void my_free (void* ptr) {
	if (!ptr) return;

	BlockHeader* block = (BlockHeader*)((char*)ptr - HEADER_SIZE);
	block->is_free = 1;

	if (block->size <= 40) {
		int index = index_fastbin(block->size);
		block->next = fastbins[index];
		fastbins[index] = block;
		return;
	}

	// Coalesce forward Patterson and Hennessy
	BlockHeader* next = (BlockHeader*)((char*)block + block->size);

	if (next->is_free == 1) {
		int index = index_regbin(next->size);
		remove_from_bin(&regbins[index], next);

		block->size += next->size;
	}

	int index = index_regbin(block->size);
	insert_sorted(&regbins[index], block);
}

void dump_bin_statistics (void) {
	printf("Fastbins:\n");
	for (int i = 0; i < NUM_FASTBINS; i++) {
		int count = 0;
		BlockHeader* cur = fastbins[i];
		while (cur) {
			count++;
			cur = cur->next;
		}
		printf( "\tFastbin %d: %d blocks\n", i, count);
	}

	printf("Regular bins:\n");
	for (int i = 0; i < NUM_REGULAR_BINS; i++) {
		int count = 0;
		BlockHeader* cur = regbins[i];
		while(cur){
			count++;
			cur = cur->next;
		}
		printf("\tRegular bin %d: %d blocks\n", i, count);
	}
}

int main () {
	printf("FASTBIN TEST\n");
	void* a = my_malloc(1); // ~32
	void* b = my_malloc(8); // ~40
	void* c = my_malloc(16); // ~24

	printf("Allocated blocks: a=%p, b=%p, c=%p\n",a, b, c);
	
	my_free(a);
	my_free(b);
	my_free(c);
	
	printf("Freed fastbin blocks.\n");
	dump_bin_statistics();

	printf("\nREGULAR BIN TEST\n");
	void* r1 = my_malloc(100); // 48-128 bin
	void* r2 = my_malloc(200); // 129-512 bin
	void* r3 = my_malloc(500); // 129-512 bin

	printf("Allocated blocks: r1=%p, r2=%p, r3=%p\n", r1, r2, r3);

	my_free(r1);
	my_free(r2);
	my_free(r3);

	printf("Freed regular blcoks.\n");
	dump_bin_statistics();

	printf("\nSPLIT TEST\n");
	void* s1 = my_malloc(3000);
	printf("Allocated block: s1=%p\n", s1);
	
	my_free(s1);
	dump_bin_statistics();

	printf("\nCOALESCING TEST\n");
	void* c1 = my_malloc(3000);
	void* c2 = my_malloc(3000);
	printf("Allocated blocks: c1=%p, c2=%p\n", c1, c2);
	
	my_free(c1);
	my_free(c2);

	printf("Freed blocks (should merge).\n");
	dump_bin_statistics();

	printf("\nFASTBIN REUSE TEST\n");
	void* f1 = my_malloc(1);
	void* f2 = my_malloc(8);

	printf("Reused blocks: f1=%p, f2=%p\n", f1, f2);

	dump_bin_statistics();

	return 0;
}
