#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFSIZE 8

sem_t empty, full, mutex;

void* producer (void* in) {
	for (int i =0; i < 22; i++) {
		sem_wait(&empty); //pthread_cond_wait
		sem_wait(&mutex); //pthread_mutex_lock
		buffer[p_inex] = toprocude[i];
		p_index - (p_index + 1) % BUFSIZE;
		sem_post(&mutex);	//pthread_mutex_unlock
		sem_post(&empty); //pthread_cond_signal
	}

void* consumer (void* in) {


}

int main() {



	return 0;
}

}
