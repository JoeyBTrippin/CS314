#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.c"

int main() {
	FILE* fp = freopen("test.txt", "w", stdout);
	if (fp == NULL) {
		perror("Failed to redirect stdout");
		return 1;
	}

	printf("FASTBIN general\n")

	void* a = my_malloc(1);
	void* b = my_malloc(2);
	void* c = my_malloc(8);

	printf("\tAllocated blocks: (a)%p, (b)%p, (c)%p\n", a, b, c);

	my_free(a);
	my_free(b);
	my_free(c);

	dump_bin_statistics();

	
	printf("\n\nFASTBIN LIFO\n");

	void* a = my_malloc(1);
	void* b = my_malloc(1);
	void* c = my_malloc(1);

	my_free(a);
	my_free(b);
	my_free(c);

	void* r1 = my_malloc(1);
	void* r2 = my_malloc(1);
	void* r3 = my_malloc(1);
	
	printf("(r1)%p ===== (c)%p\n", r1, c);
	printf("(r2)%p ===== (b)%p\n", r2, b);
	printf("(r3)%p ===== (a)%p\n", r3, a);

	
	printf("\n\nFASTBIN no coalesce\n");

	void* a = my_malloc(1);
	void* b = my_malloc(1);

	my_free(a);
	my_free(b);

	dump_bin_statistics();

	
	printf("\n\n REGULARBIN sorted insertion\n");

	void* a = my_malloc(60);
	void* b = my_malloc(80);
	void* c = my_malloc(40);
	void* d = my_malloc(100);
	void* e = my_malloc
	my_free(a);
	my_free(b);
	my_free(c);

	fclose(fp);
	 return 0;
}
