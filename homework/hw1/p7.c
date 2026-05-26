#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork failed");
		exit(1);
	}

	if (pid == 0) {
		printf("Childe: before clossing STDOUT\n");

		close(STDOUT_FILENO);

		printf("Childe: after closing STDOUT\n");	
	}
	
	wait(NULL);	
	printf("Parent: still has STDOUT open\n");

	return 0;
}
