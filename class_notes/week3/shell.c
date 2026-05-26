#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

int a = 0;
int b = 0;

void* threadmain(void* in) { // thread function
	a = 0;
	b = 0;
	b++;
	a = b;

}

int main() {

//	int pid = fork(); // child starts running here
	
	pthread_t mythread1, mythread2; // create thread variable
	pthread_create(&mythread1, NULL, threadmain, NULL); // run thread
// starts at function specified "threadmain".	
	pthread_create(&mythread2, NULL, threadmain, NULL);
/*
	if (pid != 0)
	waitpid(pid, NULL, 0);
	
	if (pid ==0) 
		execv("~/usr/bin/ls", NULL);
*/	
		
	pthread_join(mythread1, NULL);
	pthread_join(mythread2, NULL);
	printf("%d %d\n", a, b);	

//	printf("hell %d %d %p\n, pid, i, &i);

	return 0;
}
