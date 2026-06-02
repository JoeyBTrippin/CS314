Author: Joseph Blecha
Created: 5/29/2026
Class: SIUE CS314, Summer
Description: HW2 Producer/Consumer implementation as described in assignment.
	Each file is a seperate solution meant to meet the three different
	techniques ask for.

String to Produce: "I am a servant of the Secret Fire, wielder of the flame of
	 Anor. You cannot pass."

sem.c: uses semaphores as provided by semaphore.h
	COMPILE: "make sem"
	RUN: "./sem p c", replacing 'p' and "c" with number of
		producers/consumers respectively 

pthread: replaces mutex semaphore with pthread_mutex.
	COMPILE: "make pthread"
	RUN:  "./pthread p c", replacing 'p' and 'c' with number of
		producers/consumers respectively.

my_sem.c: uses my own self defined semaphore.
	COMPILE: "make my_sem"
	RUN" "./my_sem p c", replacing 'p' and 'c' with number of
		producers/consumers respectively.
