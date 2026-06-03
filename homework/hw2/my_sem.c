#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "my_sem.h"

#define BUF_SIZE 8

char items[] =
"Many that live deserve death. And some that die deserve life.";

int buffer[BUF_SIZE];
int in = 0, out = 0;
int p_index = 0;
int P = 1, C = 1;
Sem mutex, full, empty;

void* producer(void* arg) {
	long id = (long)arg;

	while(1) {
	
		sem_wait_custom(&empty);
		sem_wait_custom(&mutex);
	
		if (p_index >= sizeof(items)) {
			sem_signal_custom(&mutex);
			break;
		}

		buffer[in] = items[p_index];
		p_index++;
		printf("(P%ld) produced %c\n", id, buffer[in]);
		in = (in + 1) % BUF_SIZE;

		sem_signal_custom(&mutex);
		sem_signal_custom(&full);
	
	}

	for (int i = 0; i < C; i++) {
		sem_wait_custom(&empty);
		sem_wait_custom(&mutex);

		buffer[in] = '\0';
		in = (in + 1) % BUF_SIZE;

		sem_signal_custom(&mutex);
		sem_signal_custom(&full);

	}
	
	pthread_exit(NULL);

}

void* consumer(void* arg) {

	long id = (long)arg;

	while (1) {
		sem_wait_custom(&full);
		sem_wait_custom(&mutex);

		if (buffer[out] == '\0') {
			sem_signal_custom(&mutex);
			sem_signal_custom(&empty);
			pthread_exit(NULL);
		}

		printf("\t (C%ld) consumed %c\n", id, buffer[out]);
		out = (out + 1) % BUF_SIZE;
		
		sem_signal_custom(&mutex);
		sem_signal_custom(&empty);

	}

	pthread_exit(NULL);
}

int main (int argc, char* charv[]) {
	if (argc == 3) {
		P = atoi( charv[1]);
		C = atoi(charv[2]);
	}

	sem_init_custom(&mutex, 1);
	sem_init_custom(&full, 0); 
	sem_init_custom(&empty, BUF_SIZE);

	pthread_t prod[P], cons[C];

	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, (void*)i);

	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer,(void*) i);
	
	for (int i = 0; i < P; i++) 
		pthread_join(prod[i], NULL);

	for (int i = 0; i < C; i++)
		pthread_join(cons[i], NULL);

	pthread_exit(0);
}
