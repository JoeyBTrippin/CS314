#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

int i = 0;
#define NUMTHREADS 5 

sem_t mutex; //semaphore: protected int held in the kernel
sem_t mutex2;

void* threadstart(void* in) { // THREAD program
	int j;
	for (j=0; j < 100000; j++){
		
		sem_wait(&mutex); // order matters!!
//		sem_wait(&mutex2);	
		i++;
		sem_post(&mutex);
//		sem_post(&mutex2);

	}
	printf("j = %d\n", j);
}

void* threadstart2(void* in) { // THREAD program
	int j;
	for (j=0; j < 100000; j++){
		sem_wait(&mutex); // order matters!!!
//		sem_wait(&mutex2);	
		i++;
		sem_post(&mutex);
//		sem_post(&mutex2);

	}
	printf("j = %d\n", j);
}

int main() {
	pthread_t threads[NUMTHREADS];
	
	sem_init(&mutex, 0, 1); // initialize sumephore
	sem_init(&mutex2, 0, 1);
	
	for (int threadind = 0; threadind < NUMTHREADS; threadind++) {
		if (pthread_create(&threads[threadind], NULL,
			     threadstart, NULL) != 0)
			fprintf(stderr, "uh oh\n");
	}

	if (pthread_create(&threads[1], NULL,
			threadstart, NULL) != 0)
		printf("uh oh\n");

	for (int i = 0; i< NUMTHREADS; i++){
		pthread_join(threads[i], NULL);
	}

	printf("%d \t %p\n", i, &i);


/*
	int i = 10;

	int pid = fork()

	if (pid == 0)
		i += 10;
	else
		waitpid(pid, NULL,0);

	printf("%d\t%p\n", i, &i);

	return 0;
*/
}

