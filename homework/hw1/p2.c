#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
	int fd = open("p2.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	int pid = fork();

	if (pid == 0)
		write (fd, "Child\n", 6);
	else
		write (fd, "Parent\n", 7);
	


	return 0;
}
