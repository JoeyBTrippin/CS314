#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUF_SIZE 8
#define MAX_ITEMS 10 // how many items to process per thread
int buffer[BUF_SIZE];
int in = 0, out = 0;
int index = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
	long id = (long)arg;
	int count = 0;

	while(count < MAX_ITEMS) {
		int item = rand() % 10; // produce random numbers between 0-9
	
	pthread_mutex_lock(&mutex);
	
	while (index == BUF_SIZE)
		pthread_cond_wait(&not_full, &mutex);

	buffer[in] = item;
	printf("(P%ld) produced %d\n", id, item);
	in = (in + 1) % BUF_SIZE;
	index++;

	pthread_cond_signal(&not_empty);
	pthread_mutex_unlock(&mutex);

	count++;
	}
	
	pthread_exit(NULL);

}

void* consumer(void* arg) {
	long id = (long)arg;
	int count = 0;

	while (count < MAX_ITEMS) {
		pthread_mutex_lock(&mutex);

		while (index == 0)
			pthread_cond_wait(&not_empty, &mutex);

		int item = buffer[out];
		printf("\t (C%ld) consumed %d\n", id, item);
		out = (out + 1) % BUF_SIZE;
		index--;
		
		pthread_cond_signal(&not_full);
		pthread_mutex_unlock(&mutex);
	
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

	pthread_t prod[P], cons[C];

	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, (void*)i);

	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer,(void*) i);

	pthread_exit(0);
	return 0;
}
