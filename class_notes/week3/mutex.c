#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define PRODUCERS 1
#define CONSUMERS 1
#define BUFSIZE 5

sem_t mutex; // inizilize semaphore
sem_t empty; // keep track of empty slots in buffer
sem_t full; // kepp track of full slots in buffer

int buffer[BUFSIZE];
int c_index = 0;
int p_index = 0;
char* toproduce = "this is to be produced";

void* producer(void* in) {
	for (int i = 0; i < 22; i++){
		sem_wait(&empty);
		sem_wait(&mutex);
		buffer[p_index] = toproduce[i];
		p_index = (p_index + 1) % BUFSIZE;
		sem_post(&mutex);
		sem_post(&full);
	}
}

void* buffer(void* in) {
	while(1) {
		sem_wait(&full);
		sem_wait(&mutex);
		printf("%c", buffer[c_index]);
		c_index = (c_index + 1) % BUFSIZE;
		sem_post(&mutex);
		sem_post(&emtpy);
	}
}

int main() {
	pthread_t p[PRODUCERS];
	pthread_t c[CONSUMERS];

	sem_init(&mutex, 0, 1);
	sem_init(&empty, 0, BUFSIZE);
	sem_init(&full, 0, 0);

	for (int i = 0; i < PRODUCERS; i++) {
		pthread_create(&p[i], NULL, producer, NULL);
	}

	for (int i = 0; i < CONSUMERS; i++) {
		pthread_create(&c[i], NULL, producer, NULL);
	}
	
	for (int i =0; i < PRODUCERS; i++) {
		pthread_join(p[i], NULL);
	}	
	
	for (int i =0; i < CONSUMERS; i++) {
		pthread_join(c[i], NULL);
	}
	return 0;
}
