#include <stdio.h>
#include <stdlib.h>

typedef struct coords{
	int x;
	int y;
}coords;

#define SIZE 10

int main(){
	coords* mycoords = malloc(sizeof(coords)*SIZE);
	for (int i = 0; i < SIZE; i++){
		mycoords[i].x = i*2;
		mycoords[i].y = i*3;
	}

	FILE* out = fopen("outfile", "w");
	
	int writeout = fwrite(mycoords, 1, SIZE * sizeof(coords), out);

	return 0;
}
