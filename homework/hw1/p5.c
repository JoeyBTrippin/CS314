#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
	pid_t pid = fork();
	pid_t w;
	
	if (pid < 0) {
		perror("fork not created\n");

		exit(1);
	} else if (pid == 0) {
		printf("Child\n");
		w = wait(NULL);
		printf("Wait returned %d\n", w);
		exit(42);
	} else {
		w = wait(NULL);
		printf("Parent: PID = %d\n", pid);
		printf("Wait returned %d\n", w);
	}

	return 0;
}
