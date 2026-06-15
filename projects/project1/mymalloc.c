#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
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
#define MIN_SPLIT 16

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
	if (size <= 128) return 0;
	if (size <= 512) return 1;
	if (size <= 2048) return 2;
	return 3;
}

// Remove block from regular bin
static void remove_from_bin (BlockHeader** bin, BlockHeader* block) {
	// DEBUG
//	printf("[REGBIN REMOVE] block=%p size=%zu\n", block, block->size); 

	if (block->prev) // previous block not empty
		block->prev->next = block->next;
	else // previous block empty
		*bin = block->next;
	
	if (block->next) // next block not empty
		block->next->prev = block->prev;
	
	// Clear block
	block->next = block->prev = NULL;
}

// Insert block into regualr bin, sorted
static void insert_sorted (BlockHeader** bin, BlockHeader* block) {
	block->next = block->prev = NULL; // prevent frevious list members
	
	// DEBUG
//	printf("[REGBIN INSERT] block=%p size=%zu\n", block, block->size);

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

static BlockHeader* request_from_os (size_t total_size) {
	size_t alloc_size = ((total_size+ HEADER_SIZE + PAGE - 1)/PAGE)*PAGE;

	void* region = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, 
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if (region == MAP_FAILED) // allocation failed 
		return NULL;
	
	// Sentinel
	BlockHeader* sentinel = (BlockHeader*)((char*)region + alloc_size - HEADER_SIZE);
	sentinel->size = 0;
	sentinel->is_free = 0;
	sentinel->next = sentinel->prev = NULL;
	
	// Head
	BlockHeader* head = (BlockHeader*) region;
	head->size = total_size;
	head->is_free = 0;
	head->next = head->prev = NULL;
	
	// DEBUG
//	printf("[MMAP] region=%p alloc_size=%zu block_size = %zu\n",
//		region, alloc_size, total_size);

	// Space between Head and Sentinel
	size_t leftover = alloc_size - total_size - HEADER_SIZE;

	if (leftover >= MIN_SPLIT) {
		BlockHeader* free_block = (BlockHeader*)((char*)region +total_size);
		free_block->size = leftover;
		free_block->is_free = 1;
		free_block->next = free_block->prev = NULL;
		
		if (free_block->size <= 40) { // FASTBIN
			int idx = index_fastbin(free_block->size);
			if (idx != -1){
				free_block->next = fastbins[idx];
				fastbins[idx] = free_block;
			} else {
				int idx = index_regbin(free_block->size);
				insert_sorted(&regbins[idx], free_block);
			}
		} else { // REGBIN
			int idx = index_regbin(free_block->size);
			insert_sorted(&regbins[idx], free_block);
		}
		
		// DEBUG
//		printf("[MMAP SPLIT] free_block=%p size=%zu\n", free_block, leftover);
	}

	return head;
}

void* my_malloc (size_t size) {
	if (size == 0) return NULL;
	
	// FIND: size needed
	size_t total_size = size + HEADER_SIZE;
	total_size = (total_size + 7) & ~((size_t)7); // round to 8

	// FAST BIN path
	if (total_size <= 40) {
		int index = index_fastbin(total_size);

		if ((index != -1) && fastbins[index]) { //size is exact and bin is not empty
			BlockHeader* block = fastbins[index];
			fastbins[index] = block->next;
			block->next = NULL;
			block->is_free = 0;
			
			// DEBUG
//			printf("[FASTBIN POP] size=%zu <- fastbin[%d], block-%p\n ",
//				block->size, index, block);

			return (char*)block + HEADER_SIZE;
		}
		// fall through to regbin
	}  

	// REGULAR BIN path
	int index = index_regbin(total_size);
	BlockHeader* cur = regbins[index];

	while (cur && (cur->size < total_size)) // FIND: appropriate location
		cur = cur->next;
	
	if (cur) { // location not empty
		remove_from_bin(&regbins[index], cur);
		cur->is_free = 0;
		
		// Split
		size_t leftover = cur->size - total_size;
		if (leftover >= MIN_SPLIT) {
			BlockHeader* split = (BlockHeader*)((char*)cur + total_size);
			split->size = leftover;
			split->is_free = 1;
			split->next = split->prev = NULL;
			
			if (split->size <= 40) { // can fit in fastbin
				int idx = index_fastbin(split->size);
				if (idx != -1) { // size match
					split->next = fastbins[idx];
					fastbins[idx] = split;
				} else { // regbin
					int idx = index_regbin(split->size);
					insert_sorted(&regbins[idx], split);
				}
			}

			// DEBUG
//			printf("[SPLIT] original=%p orig_size=%zu alloc=%zu leftover=%zu split=%p\n",
//				cur, cur->size, total_size, leftover, split);

			cur->size = total_size;

		}
		// DEBUG
//		printf("[ALLOC REGBIN] block=%p size=%zu\n", cur, cur->size);

		return (char*)cur + HEADER_SIZE;
	}

	// MMAP path: reguesting memory
	BlockHeader* block = request_from_os(total_size);
	if (!block) // block does not exit
		return NULL;

	// DEBUG
//	printf("[ALLOC MMAP] block=%p size=%zu\n", block, block->size);

	return (char*)block + HEADER_SIZE;


}


void my_free (void* ptr) {
	if (!ptr) return;

	BlockHeader* block = (BlockHeader*)((char*)ptr - HEADER_SIZE);
	block->is_free = 1;
	
	// DEBUG
//	printf("[FREE] block=%p size=%zu\n", block, block->size);
	
	// FASTBIN
	if (block->size <= 40) {
		int index = index_fastbin(block->size);
		if (index != -1) { // fastbin size match
		block->next = fastbins[index];
		block->prev = NULL;
		fastbins[index] = block;
		return;
		}
		// DEBUG
//		printf("[FASTBIN PUSH] fastbin[%d] block=%p size=%zu\n",
//			index, block, block->size);
	}

	// Coalesce forward 
	BlockHeader* next = (BlockHeader*)((char*)block + block->size);

	if (!((next->is_free == 0) && (next->size == 0))) { // next is NOT the SENTINAL
		int index = index_regbin(next->size);
		remove_from_bin(&regbins[index], next);
		
		// DEBUG
//		printf("[COALESCE] block=%p(size=%zu_ + next=%p(size=%zu)\n",
//			block, block->size, next, next->size);

		block->size += next->size;
	}

	int index = index_regbin(block->size);
	insert_sorted(&regbins[index], block);
}

void dump_bin_statistics (void) {
	printf("\n===============================\n");
	printf("\t\tALLOCATOR STATE\t\t\n");
	printf("===============================\n");

	printf("FAST BINS:\n");
	for (int i = 0; i < NUM_FASTBINS; i++) {
		printf("\tFastbin[%d]: ", i);

		BlockHeader* cur = fastbins[i];
		if (!cur) {
			printf("(empty)\n");
			continue;
		}

		printf("\n");
		int count = 0;
		while (cur) {
			printf("\t\t[%p] size=%zu free=%d\n",
				(void*)cur, cur->size, cur->is_free);

			cur = cur->next;
			count++;
		}
		printf("\t\t total blocks: %d\n", count);
	}
	
	
	printf("\n\nREGULAR BINS\n");
	for (int i = 0; i < NUM_REGULAR_BINS; i++) {
		printf("\tregbin[%d]: ", i);

		BlockHeader* cur = regbins[i];
		if (!cur) {
			printf("(empty)\n");
			continue;
		}

		printf("\n");
		int count = 0;
		
		while (cur) {
			printf("\t\t[%p] size=%zu free=%d\n", (void*)cur, cur->size, cur->is_free);

			cur = cur->next;
			count++;
		}
		printf("\t\t total blocks: %d\n", count);
	}


/*
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
*/
}

int main () {
	// output to file
	FILE *fp = freopen("output.txt", "w", stdout); // Redirect stdout
	if (fp == NULL) {
		perror("Failed to redirect stdout");
		return 1;
	}

	printf("----------FAST BIN TEST--------\n");
	printf("ALLOCATE: a(1), b(2) c(8)\n");
	void* a = my_malloc(1);
	void* b = my_malloc(2);
	void* c = my_malloc(8);
	dump_bin_statistics();
	
	printf("FREE: a(1), b(2), c(8)\n");
	my_free(a);
	my_free(b);
	my_free(c);
	dump_bin_statistics();
	
	printf("\n---------REGULARE BIN TEST---------\n");
	printf("ALLOCATE: r1(200), r2(100), r3(500)\n");
	void* r1 = my_malloc(200); // 48-128 bin
	void* r2 = my_malloc(100); // 129-512 bin
	void* r3 = my_malloc(500); // 129-512 bin
	dump_bin_statistics();
	
	printf("FREE: r1(200), r2(100), r3(500)\n");
	my_free(r1);
	my_free(r2);
	my_free(r3);
	dump_bin_statistics();


	printf("\n--------SPLIT TEST--------\n");
	printf("ALLOCATE: s1(3000)\n");
	void* s1 = my_malloc(3000);
	dump_bin_statistics();

	printf("FREE: s1(3000)\n");
	my_free(s1);
	dump_bin_statistics();

	printf("\n--------COALESCING TEST--------\n");
	printf("ALLOCATE: c1(3000), c2(3000)\n");
	void* c1 = my_malloc(3000);
	void* c2 = my_malloc(3000);
	dump_bin_statistics();

	printf("FREE: c1(3000), c2(3000)\n");
	my_free(c1);
	my_free(c2);
	dump_bin_statistics();

	printf("\n--------FASTBIN REUSE TEST--------\n");
	printf("ALLOCATE: f1(1), f2(8)\n");
	void* f1 = my_malloc(1);
	void* f2 = my_malloc(8);
	dump_bin_statistics();
	
	my_free(f1);
	my_free(f2);
	dump_bin_statistics();
	fclose(fp);

	return 0;
}
