#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define BUF_SIZE 8
#define MAX_ITEMS 5 // how many iterations are wanted *(num of threads)

char items[81] = 
"I am a servant of the Secret Fire, wielder of the flame of Anor. You cannot pass.";

char buffer[BUF_SIZE];
int in = 0, out = 0;
int prod_index = 0, con_index = 0;
int P = 1, C = 1;
sem_t mutex, full, empty;

void* producer(void* arg) {
	int count = 0;
	long id = (long)arg;

	while(1) {
	
	sem_wait(&empty);
	sem_wait(&mutex);
	
	if (prod_index >= sizeof(items)){
		sem_post(&mutex);
		break;
	}

	buffer[in] = items[prod_index];
	prod_index++;
	printf("(P%ld) produced %c\n", id, buffer[in]);
	in = (in + 1) % BUF_SIZE;
	
	sem_post(&mutex);
	sem_post(&full);
	
	}
	
	for (int i = 0; i < C; i++) {
		sem_wait(&empty);
		sem_wait(&mutex);

		buffer[in] = '\0';
		in = (in + 1) % BUF_SIZE;

		sem_post(&mutex);
		sem_post(&full);

	}

	pthread_exit(NULL);

}

void* consumer(void* arg) {
	int count = 0;

	long id = (long)arg;

	while (1) {
		sem_wait(&full);
		sem_wait(&mutex);
		
//		if (con_index >= sizeof(items)){
//			sem_post(&mutex);
//			sem_post(&full);
//			pthread_exit(NULL);
//		}

		if (buffer[out] == '\0') {
			sem_post(&mutex);
			sem_post(&empty);
			pthread_exit(NULL);
		}

		printf("\t (C%ld) consumed %c\n", id, buffer[out]);
		out = (out + 1) % BUF_SIZE;
//		con_index++;



		sem_post(&mutex);
		sem_post(&empty);


	}

	pthread_exit(NULL);
}

int main (int argc, char* charv[]) {
	if (argc == 3) {
		P = atoi( charv[1]);
		C = atoi(charv[2]);
	}

	sem_init(&mutex, 0, 1);
	sem_init(&full, 0, 0); 
	sem_init(&empty, 0, BUF_SIZE);

	pthread_t prod[P], cons[C];

	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, (void*) i);

	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer, (void*) i);

	for (int i = 0; i < P; i++)
		pthread_join(prod[i], NULL);
	
	for (int i = 0; i < C; i++)
		pthread_join(cons[i], NULL);

	return 0;
}
