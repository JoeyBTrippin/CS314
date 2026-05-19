#include <stdio.h>
#include <unistd.h> // nead for shel commands, ie. fork(), exec(), wait(),...

int main() {
	int i = 123123;


	int pid = fork();

	if (pid != 0)
		wait(pid);

	if (pid == 0)
		i=999999;

	if (pid == 0)
		execv("/user/bin/ls", NULL);

	printf("hello %d %d $p\n", pid, i, &i);

	return 0;
}
