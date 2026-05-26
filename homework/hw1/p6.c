#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
	pid_t pid = fork();
	
	if (pid < 0) {
		perror("fork not created\n");

		exit(1);
	} else if (pid == 0) {
		printf("Child\n");
		exit(42);
	} else {
		waitpid(pid, NULL, 0);
		printf("Parent: PID = %d\n", pid);


	}
	return 0;
}
