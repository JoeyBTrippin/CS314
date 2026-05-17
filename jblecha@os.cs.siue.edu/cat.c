// Author: Joseph Blecha
// Class: SIUE 314 Summer
// Date: 05/16/2026
// Description: Reimplementation of the cat function

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	
	if( argc < 2) 
		return 0;
	
	FILE* file;
	char filename[256];
	char ch;
	for(int i = 1; i < argc; i++){
		
		file = fopen(argv[i], "r");
		if (file == NULL){
			fprintf(stderr, "./cat: %s: No such file or directory\n", argv[i]);
			continue;
		}
		while((ch = fgetc(file)) != EOF)
			fprintf(stdout, "%c", ch);

		fclose(file);
	}


	return 0;
}
