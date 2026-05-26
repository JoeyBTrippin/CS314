#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork failed\n");
		exit(1);
	}

	if (pid == 0) {
		// execl(path, arg0, arg1, ..., NULL)
//		execl("/bin/ls", "ls", "-l", (char*) NULL);
		
		// execle(path, arg0, ..., NULL, envp)
//		char* myenv[] = {"MYVAR=123", NULL};
//		execle("/bin/ls", "ls", "-l", (char*)NULL, myenv);

		// execlp(file, arg0, ..., NULL) (searches PATHh)
//		execlp("ls", "ls", "-l", (char*) NULL);

		// execv(path, argv[])
//		char* args[] = {"ls", "-l", NULL};
//		execv("/bin/ls", args);

		// execvp(filem, argv[]) (searches PATH)
//		char* args2[] = {"ls", "-l", NULL};
//		execvp("ls", args2);

		// execvpe(file, argv[]. envp) (GNU extension)
		char* args3[] = {"ls", "-l", NULL};
	        char* envp[] = {"MYVAR=456", NULL};
     		execvpe("ls", args3, envp);	       

		perror("exec failed");
		exit(1);
	}

	printf("Parent continues running./n");
	return 0;


}
