#include "schedulers.h"

/*
 (int) fcfs takes a (settings *) info containing the ready and waiting queues,
 along with the (int) cpu_busy, io_busy and file_read_done states. 

 When fcfs should continuously run, 0 is returned. Otherwise, when CPU scheduling
 must be stopped, 1 is returned. 
*/
int fcfs(settings *info) {
	int res; // Semaphore timed wait result
	doubleLL *readyQueue, *waitingQueue; // Keep copies of respective queue pointers
	int cpu_busy, io_busy, file_read_done;
	// Get shared data from info 
	sem_wait(&accessSem);
    readyQueue = info->readyQueue;
    waitingQueue = info->waitingQueue;
    cpu_busy = info->cpu_busy;
    io_busy = info->io_busy;
    file_read_done = info->file_read_done;
    sem_post(&accessSem);

    if (!readyQueue && !cpu_busy && !waitingQueue && !io_busy && file_read_done) return 1;

	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	time.tv_sec += 1; // Set max wait time to 1 second
	
	res = sem_timedwait(&queueSem[0], &time);
	if (res == -1 && errno == ETIMEDOUT) return 0;
	// When there is an item in the ready queue
	sem_wait(&accessSem); // Access the ready queue
	// Get the current PCB from the linked list
	info->cpu_busy = 1;
	PCB *current = removeHeadDLL(&info->readyQueue);
	sem_post(&accessSem); // Reinstate access to ready queue
	// Add total waiting time in ready queue to process PCB
	clock_gettime(CLOCK_MONOTONIC, &current->r_end);
	double waitTime = current->r_end.tv_sec - current->r_begin.tv_sec;
	waitTime += (current->r_end.tv_nsec - current->r_begin.tv_nsec) / 1000000000.0;
	waitTime *= 1000; // Convert to ms
	useconds_t processTime = current->jobs[current->current];
	// Increment job index
	current->current += 1;
	current->waitTime += waitTime; // Add current wait time to PCB time
	usleep(processTime * 1000); // "process" for processTime ms
	if (current->current == current->count) {
		// Set the finish time for the current job
		clock_gettime(CLOCK_MONOTONIC, &current->ts_end);
		// We processed the final job, so free PCB
		double runtime = current->ts_end.tv_sec - current->ts_begin.tv_sec;
		runtime += (current->ts_end.tv_nsec - current->ts_begin.tv_nsec) / 1000000000.0;
		runtime *= 1000; //  Convert from seconds to ms 
		// Update info members - only used by scheduler while scheduler is 
		// running, so no protection needed
		info->elapsed += runtime;
		info->completed += 1;
		info->totalWaitTime += current->waitTime; // Add total wait times
		free(current->jobs);
		free(current);
	} else {
		// Place the current job into the waiting queue
		sem_wait(&accessSem); // Get access to waiting queue
		insertTailDLL(&info->waitingQueue, current);
		sem_post(&accessSem); // Reinstate access to waiting queue
		sem_post(&queueSem[1]); // Signal that IO time available in queue 
	}
	sem_wait(&accessSem);
	info->cpu_busy = 0;
	sem_post(&accessSem);
	return 0; 
}