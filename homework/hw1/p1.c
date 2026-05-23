#include <stdio.h>
#include <unistd.h>

int main(){
	int x = 100;
	int pid;

	pid = fork();
	printf("Value of x after fork: pid %d, x = %d\n", pid, x);
	if (pid == 0){ // child only
		x = 150;
	}
	else  // parent only
		x = 50;
	
	printf("Value of x after value changed: pid %d, x = %d\n", pid, x);

	return 0;
}
