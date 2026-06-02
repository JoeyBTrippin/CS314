#ifndef MYSEM_H
#define MYSEM_H

#include <pthread.h>

typedef struct {
	int count;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}Sem;

void sem_init_custom(Sem* s, int value) {
	s->count = value;
	pthread_mutex_init(&s->mutex, NULL);
	pthread_cond_init(&s->cond, NULL);
}

void sem_wait_custom(Sem *s) {
	pthread_mutex_lock(&s->mutex);
	while (s->count == 0)
		pthread_cond_wait(&s->cond, &s->mutex);
	s->count--;
	pthread_mutex_unlock(&s->mutex);
}

void sem_signal_custom(Sem* s) {
	pthread_mutex_lock(&s->mutex);
	s->count++;
	pthread_cond_signal(&s->cond);
	pthread_mutex_unlock(&s->mutex);
}

#endif
