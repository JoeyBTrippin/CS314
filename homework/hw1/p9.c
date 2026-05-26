#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int file[2]; // pipe

// Writer thread
void* thread1(void* arg) {
	const char* msg = "Hello from thread 1\n";
	write(file[1], msg, sizeof("Hello from thread 1\n") - 1);

	close(file[1]);
	return NULL;
}

// Reader thread
void* thread2(void* arg) {
	char ch;
	while (read(file[0], &ch, 1) > 0) {
		printf("Thread 2 got: %c\n", ch);
	}

	close(file[0]);
	return NULL;
}

int main() {
	pthread_t t1, t2;
	
	if (pipe(file) == -1) {
		perror("pipe failed");
		exit(1);
	}	

	pthread_create(&t1, NULL, thread1, NULL);
	pthread_create(&t2, NULL, thread2, NULL);

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	return 0;
}
