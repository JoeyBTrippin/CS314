#include <stdio.h>
//#include <stdlib.h>

#define SIZE 10

char mymem[1024*1024];

void* mymalloc(int bytes) {
	static int base = 0;
	void* ret = base+mymem;
	base += bytes;
	return ret;
	

}

int main() {
	int* ints = mymalloc(sizeof(int)*SIZE);
	int* ints2 = mymalloc(sizeof(int)*SIZE);

	for (int i = 0; i < SIZE; i++) {
		ints[i] = i*2;
	}

	for (int i = 0; i < SIZE; i++) {
		ints2[i] = i*2;
	}

	for (int i = 0; i < SIZE; i++) {
		printf(" %d (%p)\t ", ints[i], &(ints[i]));
	}
	printf("\n");

	for (int i = 0; i < SIZE; i++) {
		printf(" %d (%p)\t ", ints2[i], &(ints2[i]));
	}

//	free(ints);



	return 0;
}
