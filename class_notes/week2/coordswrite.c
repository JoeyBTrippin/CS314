#include <stdio.h>

typedef struct coords{
	int x;
	int y;
}coords;



int main(){
	coords mycoords;

	FILE* in = fopen("outfile", "r");
	
	while (fread(&mycoords, 1, sizeof(coords), in) == sizeof(coords))
		printf("x = %d, y = %d\n", mycoords.x, mycoords.y);

	return 0;
}
