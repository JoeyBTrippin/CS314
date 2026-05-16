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
	char line[256];
	for(int i = 1; i < argc; i++){
		char* filename = argv[i];
		
		file = fopen(filename, "r");
		if (file == NULL){
			printf("./cat: %s: No such file or directory\n", filename);
			continue;
		}
		while(fgets(line, sizeof(line), file))
			printf("%s", line);

		fclose(file);
	}
	

	return 0;
}
