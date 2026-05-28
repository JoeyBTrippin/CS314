#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem1, sem2;

int x = 0;

void* mainthread1(void* in) {
for (int i = 0; i < 5; i++) {
		sem_wait(&sem1);
		x += 1;
		printf("thread1: x = %d\n", x);
		sem_post(&sem2);
	}
	return NULL;
}

void* mainthread2(void* in) {

	for (int i = 0; i < 5; i++) {
		sem_wait(&sem2);
		x += 1;
		printf("thread2: x = %d\n", x);
		sem_post(&sem1);
	}
	return NULL;
}

int main() {
	pthread_t thread1, thread2;

	sem_init(&sem1, 0, 1);
	sem_init(&sem2, 0, 0);

	pthread_create(&thread1, NULL, mainthread1, NULL);
	pthread_create(&thread2, NULL, mainthread2, NULL);	
	
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);	

	return 0;
}
