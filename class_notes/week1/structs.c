#include <stdio.h>

// struct def
typedef struct coords{
	int x;
	int y;
}coords;

int main() {
	
	coords mycoords;
	mycoords.x = 10;
	mycoords.y = 123;
	
	// Open file
	FILE* out = fopen("outfile", "w");
	
	// Write to open file
	fwrite(&mycoords, 1, sizeof(coords), out);
		

	return 0;
}
