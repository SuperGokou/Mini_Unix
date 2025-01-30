#ifndef SCHEDULERS_H
#define SCHEDULERS_H

#include "doublell.h"
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/*
 The PCB struct contains information about specific processes. Below is a
 definition of all PCB members:

 * (int) PID/PR - PID contains the current process's ID, and PR contains the
 priority of the current proces.
 * (int) count - The number of jobs assigned to the process. This should be
 equal to the length of the (long *)jobs array.
 * (int) current - The current index of the job being handled. This value should
 be incremented upon job completion.
 * (long *) jobs - A dynamically allocated array containing the number of jobs
 to run for this process. Even indexed jobs are CPU bursts, and odd indexed
 jobs are IO bursts. 
 * (struct timespec) ts_begin/ts_end - ts_begin contains the time when the
 process was first scheduled, and ts_end contains the time when the process
 finishes.
 * (double) waitTime - The current amount of time for the process that has been
 spent waiting in the ready queue.
*/

typedef struct PCB {
    int PID, PR;
    int count;
    int current;
    long *jobs;
    struct timespec ts_begin, ts_end;
    struct timespec r_begin, r_end; // Used for wait metrics
    double waitTime; // Amount of time process has been sitting in ready queue
} PCB;

/*
 The settings struct is used to track information about the scheduler across
 multiple threads. Below is a definition of all settings members:

 * (int) algorithm - defines the algorithm used in the program. The map of
 integers to algorithms is shown below:
 0 - FIFO/FCFS
 1 - SJF
 2 - PR
 3 - RR
 * (char *) inputFile - path to the input file to read and fill processes.
 * (useconds_t) quantum - defines the round robin quantum time
 * (doubleLL *) waitingQueue/readyQueue - Pointers representing the current
 doubly linked lists for the ready queue and waiting queue.
 * (int) file_read_done/cpu_sch_done/io_sys_done/cpu_busy/io_busy - Variables
 used to determine the current global state of the program. These variables are
 used to determine if the program has finished processing jobs.
 * (double) elapsed/totalWaitTime - elapsed contains the current amount of time
 in ms that have passed completing each job. This member will contain the final
 amount of time that has passed once scheduling is complete. totalWaitTime 
 contains the total wait time across all PCBs for jobs that have been waiting
 in the ready queue.
 * (int) completed - The number of jobs that have been processed.

*/
typedef struct settings {
    int algorithm;
    char *inputFile;
    useconds_t quantum;
    doubleLL *waitingQueue;
    doubleLL *readyQueue;
    int file_read_done, cpu_sch_done, io_sys_done, cpu_busy, io_busy;
    double elapsed, totalWaitTime;
    int completed;
} settings;


// Use the same semaphores in the scheduling files as in main.c
extern sem_t accessSem;
extern sem_t queueSem[2];

int fcfs(settings *info);
int sjf(settings *info);
int rr(settings *info);
int pr(settings *info);

#endif
