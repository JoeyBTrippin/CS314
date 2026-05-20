#include <stdio.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char* argv[]) {
	int i = 0, startflag = -1, endflag = -1, pid;
	struct timespec start, end;
	double totaltime = 0;
	
	
	
	while(i < 100) {
	
	startflag = clock_gettime(CLOCK_MONOTONIC, &start);
	if(startflag)
		printf("failed to start clock");

	pid = fork();
	if (pid == 0)
		_exit(0);
	endflag = clock_gettime(CLOCK_MONOTONIC, &end);

	totaltime += end.tv_sec - start.tv_sec;

	if(endflag)
		printf("Failed to end clock");

	i++;
	}
	
	printf("Average time for %d number of runs: %f\n", i, totaltime/i);
	return 0;
}
