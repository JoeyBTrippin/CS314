#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define BUF_SIZE 8
#define MAX_ITEMS 5 // how many iterations are wanted *(num of threads)

int buffer[BUF_SIZE];
int in = 0, out = 0;
int count = 0;
sem_t mutex, full, empty;

void* producer(void* arg) {
	int count = 0;
	long id = (long)arg;

	while(count < MAX_ITEMS) {
		int item = rand() % 10; // produce random numbers between 0-9
	
	sem_wait(&empty);
	sem_wait(&mutex);

	buffer[in] = item;
	printf("(P%ld) produced %d\n", id, item);
	in = (in + 1) % BUF_SIZE;
	
	sem_post(&mutex);
	sem_post(&full);
	
	count ++;
	}
	
	pthread_exit(NULL);

}

void* consumer(void* arg) {
	int count = 0;

	long id = (long)arg;

	while (count < MAX_ITEMS) {
		sem_wait(&full);
		sem_wait(&mutex);

		int item = buffer[out];
		printf("\t (C%ld) consumed %d\n", id, item);
		out = (out + 1) % BUF_SIZE;
		
		sem_post(&mutex);
		sem_post(&empty);

		count++;

	}

	pthread_exit(NULL);
}

int main (int argc, char* charv[]) {
	int P, C;
	if (argc == 3) {
		P = atoi( charv[1]);
		C = atoi(charv[2]);
	}

	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0); 
	sem_init(&empty, 0, BUF_SIZE);

	pthread_t prod[P], cons[C];

	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, (void*)i);

	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer,(void*) i);

	pthread_exit(0);
}
