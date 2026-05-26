#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {
	int file[2];
	
	// file[0] = read end. file[1] = write end	
	if (pipe(file) == -1) {
		perror("Error creating pipe\n");
		exit(1);
	}

	// Child 1
	pid_t child1 = fork();
	if (child1 == -1) {
		perror("Error creating Child1\n");
		exit(1);
	}
	
	if (child1 == 0) {
		close(file[0]);
		dup2(file[1], STDOUT_FILENO);
		close(file[1]);

		printf("Hello from Child 1\n");
		fflush(stdout);

		exit(0);
	}
	

	// Child 2
	pid_t child2 = fork();
	if (child2 == -1) {
		perror("Error creating Child2\n");
		exit(1);
	}

	if (child2 == 0) {
		close(file[1]);
		dup2(file[0], STDIN_FILENO);
		close(file[0]);
		
		char buffer[256];	
		while (fgets(buffer, sizeof(buffer), stdin)) {
			printf("Child 2 receiving: %s\n", buffer);
		}

		exit(0);
	}

	close(file[0]);
	close(file[1]);

	waitpid(child1, NULL, 0);
	waitpid(child2, NULL, 0);

	return 0;
}
