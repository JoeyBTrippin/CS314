#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int main() {
	int fd = open("p2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	int pid = fork();

	if (pid == 0){
		write (fd, "Child\n", 6);
		exit(1);
	}
	else
		write (fd, "Parent\n", 7);
	
	close(fd);

	return 0;
}
