#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE 8

char items[81] =
"I am a servant of the Secret Fire, wielder of the flame of Anor. You cannot pass.";

int buffer[BUF_SIZE];
int in = 0, out = 0;
int p_index = 0;
int count = 0;
int P= 1, C = 1;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
	long id = (long)arg;

	while(1) {
		pthread_mutex_lock(&mutex);
	
		while (count == BUF_SIZE)
			pthread_cond_wait(&not_full, &mutex);

		if (p_index >= strlen(items)) {
			pthread_mutex_unlock(&mutex);
			break;
		}

		buffer[in] = items[p_index];
		p_index++;
		printf("(P%ld) produced %c\n", id, buffer[in]);
		in = (in + 1) % BUF_SIZE;
		count++;

		pthread_cond_signal(&not_empty);
		pthread_mutex_unlock(&mutex);

	}
	
	for (int i = 0; i < C; i++) {
		pthread_mutex_lock(&mutex);
		
		while (count == BUF_SIZE)
			pthread_cond_wait(&not_full, &mutex);

		buffer[in] = '\0';
		in = (in + 1) % BUF_SIZE;
		count++;

		pthread_cond_signal(&not_empty);
		pthread_mutex_unlock(&mutex);
	}

	pthread_exit(NULL);

}

void* consumer(void* arg) {
	long id = (long)arg;

	while (1) {
		pthread_mutex_lock(&mutex);

		while (count == 0)
			pthread_cond_wait(&not_empty, &mutex);
		
		if (buffer[out] == '\0') {
			out = (out + 1) % BUF_SIZE;
			count--;
			
			pthread_cond_signal(&not_full);
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}

		printf("\t (C%ld) consumed %c\n", id, buffer[out]);
		out = (out + 1) % BUF_SIZE;
		count--;
		
		pthread_cond_signal(&not_full);
		pthread_mutex_unlock(&mutex);
	
	}
	
	pthread_exit(NULL);
}

int main (int argc, char* charv[]) {
	if (argc == 3) {
		P = atoi( charv[1]);
		C = atoi(charv[2]);
	}

	pthread_t prod[P], cons[C];

	for (int i = 0; i < P; i++)
		pthread_create(&prod[i], NULL, producer, (void*)i);

	for (int i = 0; i < C; i++)
		pthread_create(&cons[i], NULL, consumer,(void*) i);

	for (int i = 0; i < P; i++)
		pthread_join(prod[i], NULL);
	
	for (int i = 0; i < C; i++)
		pthread_join(cons[i], NULL);

	return 0;
}
