Author: Joseph Blecha
Created: 5/29/2026
Class: SIUE CS314, Summer
Description: HW2 Producer/Consumer implementation as described in assignment.
	Each file is a seperate solution meant to meet the three different
	techniques ask for.

String to Produce:

sem.c: uses semaphores as provided by semaphore.h.
	PRODUCES: "I am a servant of the Secret Fire, wielder of the flam of Anor."
	COMPILE: "make sem"
	RUN: "./sem p c", replacing 'p' and "c" with number of
		producers/consumers respectively 

pthread: replaces mutex semaphore with pthread_mutex.
	PRODUCES: "All we have to decide is what to do with the time that is given."
	COMPILE: "make pthread"
	RUN:  "./pthread p c", replacing 'p' and 'c' with number of
		producers/consumers respectively.

my_sem.c: uses my own self defined semaphore.
	PRODUCES: "Many that live deserve death. And some that die deserve life."
	COMPILE: "make my_sem"
	RUN: "./my_sem p c", replacing 'p' and 'c' with number of
		producers/consumers respectively.
	my_sem.h: contains the implementation file of my custom semaphore.
