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
//	size_t alloc_size = total_size + HEADER_SIZE;

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
	
//	return head;
	// DEBUG
//	printf("[MMAP] region=%p alloc_size=%zu block_size = %zu\n",
//		region, alloc_size, total_size);

	// Space between Head and Sentinel
	size_t leftover = alloc_size - total_size - HEADER_SIZE;

	if (leftover >= MIN_SPLIT) {
		BlockHeader* free_block = (BlockHeader*)((char*)head +total_size);
		free_block->size = leftover;
		free_block->is_free = 1;
		free_block->next = free_block->prev = NULL;
		
		int idx = index_fastbin(free_block->size);
		if (idx != -1){
			free_block->next = fastbins[idx];
			fastbins[idx] = free_block;
		} else {
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
			int idx = index_fastbin(split->size);		
			if (idx != -1) { // size match
				split->next = fastbins[idx];
				fastbins[idx] = split;
			} else { // regbin
				int idx = index_regbin(split->size);
				insert_sorted(&regbins[idx], split);
			}

			cur->size = total_size;
		}

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
	 
	return;
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

}

int test () {
	
	printf("--------FAST BIN TEST--------\n");
	printf("\nALLOCATE: a(1), b(8) c(16)");
	void* a = my_malloc(1);
	void* b = my_malloc(8);
	void* c = my_malloc(16);
	dump_bin_statistics();

	printf("\nFREE: a(1), b(8), c(16)");
	my_free(a);
	my_free(b);
	my_free(c);
	dump_bin_statistics();
	printf("only 'a'and 'b' go to fast bin\n");

	printf("\n-----LIFO REUSE-----\n");

	printf("\nALLOCATE: r1(1), r2(1), r3(1)");
	void* r1 = my_malloc(1);
	void* r2 = my_malloc(1);
	void* r3 = my_malloc(1);

	printf("\nFREE: r1(1)");
	my_free(r1);
	dump_bin_statistics();
	
	printf("\nFREE: r2(1)");
	my_free(r2);
	dump_bin_statistics();

	printf("\nFREE: r3(1)");
	my_free(r3);
	dump_bin_statistics();
	printf("freed bins are placed in front of bin\n");
	
	printf("\nALLOCATE: r1(1)");
	r1 = my_malloc(1);
	dump_bin_statistics();
	printf("\nALLOCATE: r2(1)");
	r2 = my_malloc(1);
	dump_bin_statistics();
	printf("\nALLOCATE: r3(1)");
	printf("allocated space is taken from front of bin\n");
	printf("when fast bin not available, takes from regbin before\n");
	printf("allocating new memory.\n");
	

	printf("\n\n\n--------REGULAR BIN--------\n");
	printf("-----SORTED TEST-----\n");
	
	printf("ALLOCATE: p1(100), p2(80), p3(40), p4(120), p5(20)\n");
	void* p1 = my_malloc(100);
	void* p2 = my_malloc(80);
	void* p3 = my_malloc(40);
	void* p4 = my_malloc(120);
	void* p5 = my_malloc(20);
	
	printf("FREE: p1(100), p2(80), p3(40), p4(120), p5(20)\n");
	my_free(p1);
	my_free(p2);
	my_free(p3);
	my_free(p4);
	my_free(p5);
	dump_bin_statistics();
	printf("dispite allocation not being in size order, regbin is sorted by size\n");

	
	printf("\n\n-----SPLITTING/COALESCING-----\n");
	printf("ALLOCATE: s1(3000)\n");
	void* s1 = my_malloc(3000);
	dump_bin_statistics();
	printf("total allocated size = 3040\n");
	printf("leftover goes to regbin[2])\n");
	
	printf("FREE: s1(3000)\n");
	my_free(s1);
	dump_bin_statistics();
	printf("once freed it is coalesced\n");

	printf("ALLOCATE: s2(4048)\n");
	void* s2 = my_malloc(4000);
	printf("FREE: s2(4048)\n");
	my_free(s2);
	dump_bin_statistics();
	printf("split to fast bin");

	return 0;
}

int main () {
	/* DO NOTE CHANGE */
	// redurecting stdout to output.txt
	int saved_stdout = dup(fileno(stdout));
	FILE *fp = freopen("output.txt", "w", stdout); 
	if (fp == NULL) {
		perror("Failed to redirect stdout");
		return 1;
	}
	test(); // Run test code
	fflush(stdout);
	dup2(saved_stdout, fileno(stdout));
	close(saved_stdout);
	//  Standard out restored 
	
	/* Below is for instructer use */
	

	
	return 0;

}
