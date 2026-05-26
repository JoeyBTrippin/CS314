#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM 10

sem_t mutex; // inizilize semaphore
sem_t signalsem, signalsem2; //
sem_t barrier;

int a = 0;
int count = 0;

void* threadmain(void* in) {
	for (int i =0; i < 10000; i++){
	sem_wait(&mutex); // wait 
	
	a++;
	
	sem_post(&mutex); // post
	}
}

void* threadmain1(void* in) {
	printf("1 ");
	sem_post(&signalsem); // post 1
}

void * threadmain2 (void* in) {
	sem_wait(&signalsem); // wait 1
	printf("2 ");
	sem_post(&signalsem2);// post 2
}

void* threadmain3 (void* in) {
	sem_wait(&signalsem2); // wait 2
	printf("3 ");
}

void* threadmain4 (void* in) {
	sem_wait (&mutex);
	count++;
	sem_post(&mutex);

	printf("working on frame 1\n");

	if (count == NUM) sem_post(&barrier);
	sem_wait(&barrier);
	sem_post(&barrier);

	printf("working on frame 2\n");
}

int main() {
	pthread_t mythread[NUM];

	sem_init(&mutex, 0, 1);
	sem_init(&barrier, 0, 0);

	for (int i = 0; i < NUM; i++) {
		pthread_create(&mythread[i], NULL, threadmain4, NULL);
	}

	for (int i = 0; i < NUM; i++) {
		pthread_join(mythread[i], NULL);
	}
/*
	pthread_t mythread1, mythread2; // initialize threads

	sem_init(&mutex, 0, 1); // define semaphore
	sem_init(&signalsem, 0, 0); //
	sem_init(&signalsem2, 0, 0); //
	pthread_create(&mythread1, NULL, threadmain1, NULL); // define threads
	pthread_create(&mythread2, NULL, threadmain2, NULL); //
	pthread_create(&mythread1, NULL, threadmain3, NULL); //
	pthread_join(mythread1, NULL); // wait for thread
	pthread_join(mythread2, NULL); //
	
//	printf("%d\n", a);
	printf("\n");
*/
	return 0;
}
