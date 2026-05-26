#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
	int pid = fork();

	if (pid ==0){
		printf("hello\n");
		exit(1);
	}
	else {
		sleep(1);
		printf("goodbye\n");		
		
	}

	return 0;
}
