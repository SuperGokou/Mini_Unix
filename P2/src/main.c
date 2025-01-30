#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "schedulers.h"

// Return value error codes
#define ERR_ARGS 1 // Invalid/insufficient arguments
#define SEM_ERR 2 // Error related to semaphore initialization/operation
#define FILE_ERR 3 // Error reading/opening file

#define ALGO_COUNT 4 // Number of algorithms supported in this program

/*
 Semaphore indexes:
 * 0 - Semaphores relating to the ready queue
 * 1 - Semaphores relating to the waiting queue

 The access semaphores are used to obtain rights to access information about
 the queues or operate on them one at a time to avoid race conditions.

 The queue semapores are used to determine whether or not a queue has an item
 sitting in it or not. The queue semaphores are effectively a count for the
 number of items in a doubly linked list (doubleLL).
*/
sem_t accessSem, queueSem[2];

/*
 (int) algorithmMap takes some given algorithm (char *) algo and compares it to
 the list of algorithms defined in (const char **) algorithms. When a known 
 scheduling algorithm is discovered, its code is returned.

 Algorithm code mappings:
 * 0 - FIFO/FCFS
 * 1 - SJF
 * 2 - PR
 * 3 - RR
*/
int algorithmMap(char *algo, const char **algorithms) {
    int algorithm = -1;
    for (int i = 0; i < ALGO_COUNT; i++) {
        if (!strcmp(algo, algorithms[i])) {
            algorithm = i;
            break;
        }
    }
    return algorithm;
}

/*
 (void *) fileReadRoutine takes a (void *) infoArg containing resources
 shared between threads. This function opens the file specified at
 infoArg->inputFile and parses it to create PCBs to place onto the ready queue.
 This function should be run under a pthread. 

 On success, (void *) fileReadRoutine will exit with a code of 0. Otherwise
 this thread terminates with an exit code FILE_ERR.
*/
void *fileReadRoutine(void *infoArg) {
    int currPID = 0;
    settings *info = (settings *)infoArg;
    doubleLL **readyQueue = &info->readyQueue;
    // Open the file detailed in the settings struct
    FILE *fp = fopen(info->inputFile, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", info->inputFile);
        info->file_read_done = 1;
        pthread_exit((void *)FILE_ERR);
    }
    char *line = NULL; // Use heap memory for getline
    size_t linelen = 0;
    char *currentToken = NULL;
    while (getline(&line, &linelen, fp) != -1) {
        currentToken = strtok(line, " "); // Separate by spaces
        // Check what the first token is for this line
        if (!strncmp(currentToken, "proc", 4)) {
            // Create and populate new PCB with information from file
            PCB *newPCB = malloc(sizeof(PCB));
            currentToken = strtok(NULL, " ");
            newPCB->PR = strtol(currentToken, NULL, 10);
            // Verify given priority is between 1 and 10
            if (newPCB->PR > 10) newPCB->PR = 10;
            else if (newPCB->PR < 1) newPCB->PR = 1;
            currentToken = strtok(NULL, " ");
            newPCB->count = strtol(currentToken, NULL, 10);
            newPCB->jobs = malloc(sizeof(long) * newPCB->count);
            newPCB->current = 0;
            newPCB->PID = currPID++; // Assign process ID to PID
            newPCB->waitTime = 0.0;
            // Will assume that the file is in the correct format
            for (int i = 0; i < newPCB->count; i++) {
                // Grab all successive tokens for job times
                currentToken = strtok(NULL, " ");
                newPCB->jobs[i] = strtol(currentToken, NULL, 10);
            }
            // Insert the PCB into the doubly linked list
            sem_wait(&accessSem);
            // Set the start time for the current PCB
            clock_gettime(CLOCK_MONOTONIC, &newPCB->ts_begin);
            clock_gettime(CLOCK_MONOTONIC, &newPCB->r_begin);
            insertTailDLL(readyQueue, newPCB);
            sem_post(&accessSem);
            sem_post(&queueSem[0]); // Signal that ready queue has pcb
        } else if (!strncmp(currentToken, "sleep", 5)) {
            currentToken = strtok(NULL, " ");
            useconds_t sleepTime = strtol(currentToken, NULL, 10);
            usleep(sleepTime * 1000); // Convert sleep time from us to ms
        } else if (!strncmp(currentToken, "stop", 4)) {
            break; // Exit main loop and close
        }
    }
    free(line); // Free line on heap
    fclose(fp); // Close file after processing
    info->file_read_done = 1;
    pthread_exit(0);
}

/*
 (void *) ioRoutine takes a (void *) infoArg containing information for the
 ready/wait queues. (void *) ioRoutine will wait for items to appear in the
 waiting queue and processes them by sleeping for their given period of time.
 This function always grabs items from the waiting queue in FIFO order.

 Thread running function will always have an exit code of 0.
*/
void *ioRoutine(void *infoArg) {
    settings *info = (settings *) infoArg;
    struct timespec time;
    doubleLL *readyQueue, *waitingQueue;
    while (1) {
        int res; // Timed wait semaphore result
        int file_read_done, cpu_busy;
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 1; // Set max wait time to 1 second
        // Check if ioThread should close
        sem_wait(&accessSem); // Access settings struct
        readyQueue = info->readyQueue;
        waitingQueue = info->waitingQueue;
        file_read_done = info->file_read_done;
        cpu_busy = info->cpu_busy;
        sem_post(&accessSem); // Reinstate access to settings struct
        if (!readyQueue && !cpu_busy && !waitingQueue && file_read_done) break;
        // Wait for an IO item to be available in the queue
        res = sem_timedwait(&queueSem[1], &time);
        if (res == -1 && errno == ETIMEDOUT) {
            continue;
        }
        sem_wait(&accessSem); // Wait for lock to io queue
        info->io_busy = 1; // Signal IO is busy
        PCB *currentPCB = removeHeadDLL(&info->waitingQueue); // Grab PCB in FIFO order
        sem_post(&accessSem); // Give back access to wait queue
        // Sleep for the current wait time in current PCB
        useconds_t sleepTime = currentPCB->jobs[currentPCB->current];
        usleep(sleepTime * 1000); // Sleep for sleepTime in ms
        // Update currentPCB state
        currentPCB->current += 1;
        sem_wait(&accessSem); // Obtain access to wait queue once again
        clock_gettime(CLOCK_MONOTONIC, &currentPCB->r_begin); // Start timer for R Q
        insertTailDLL(&info->readyQueue, currentPCB); // Insert updated PCB back in FIFO order
        info->io_busy = 0; // Mark IO as free
        sem_post(&accessSem);
        sem_post(&queueSem[0]); // Signal process is available
    }
    sem_wait(&accessSem);
    info->io_sys_done = 1;
    sem_post(&accessSem);
    pthread_exit(0);
}

/*
 (void *) cpuRoutine takes a (void *) infoArg containing the algorithm to use,
 and the queues to use in the scheduling function. The algorithms array holds
 a list of function pointers corresponding to the map of algorithms defined
 in the comment block above algorithmMap. 
*/
void *cpuRoutine(void *infoArg) {
    int (*algorithms[])(settings *) = {
        fcfs,
        sjf,
        pr,
        rr
    };
    settings *info = (settings *)infoArg;
    int algorithm = info->algorithm; // Get algorithm to use from settings
    int result = 0;
    while (1) {
        // Call the currently set algorithm and check its return value.
        result = algorithms[algorithm](info);
        if (result) break;
    }
    info->cpu_sch_done = 1; // Mark scheduling as finished
    pthread_exit(0);
}

/*
 (int) main parses arguments from (char **) argv, initializes semaphores and
 spins up all three threads. After joining all three threads, main prints out
 statistics about the algorithm.

 On success, program will close with an exit core of 0. Otherwise, the exit
 code will contain an error code as defined at the top of this document.
*/
int main(int argc, char **argv) {
    const char *algo_flag = "-alg";
    const char *quantum_flag = "-quantum";
    const char *input_flag = "-input";
    const char *algorithms[ALGO_COUNT] = {"FIFO", "SJF", "PR", "RR"};
    int hasQuantum = 0; // Flag must be set to 1 if RR algorithm is used

    // Create settings struct to pass around
    settings info;
    memset(&info, 0, sizeof(settings)); // Zero out settings
    if (argc < 5) {
        printf("\nUsage: ./prog -alg [FIFO|SJF|PR|RR] ");
        printf("[-quantum [integer(ms)]] -input [file name]\n\n");
        return ERR_ARGS;
    }
    for (int i = 1; i < argc; i++) {
        // Get the current algorithm to use
        if (!strcmp(argv[i], algo_flag)) {
            // Go to next argument, error if doesn't exist
            if (++i >= argc) {
                fprintf(stderr, "Error: Not enough arguments\n");
                return ERR_ARGS;
            }
            info.algorithm = algorithmMap(argv[i], algorithms);
            if (info.algorithm == -1) {
                fprintf(stderr, "Error: Invalid algorithm name\n");
                return ERR_ARGS;
            }
        // Get the quantum to use (when applicable)
        } else if (!strcmp(argv[i], quantum_flag)) {
            // Go to next argument, error if doesn't exist
            if (++i >= argc) {
                fprintf(stderr, "Error: Not enough arguments\n");
                return ERR_ARGS;
            }
            info.quantum = strtol(argv[i], NULL, 10); // Convert argument to long
            hasQuantum = 1;
        // Get the input file to use
        } else if (!strcmp(argv[i], input_flag)) {
            if (++i >= argc) {
                fprintf(stderr, "Error: Not enough arguments\n");
                return ERR_ARGS;
            }
            info.inputFile = argv[i];
        }
    }
    if (info.algorithm == 3 && hasQuantum == 0) {
        fprintf(stderr, "Error: RR requires a quantum\n");
        return ERR_ARGS;
    }
    // Initialize semaphores
    int s_res = 0;
    // Initialize queue semaphores
    for (int i = 0; i < 2; i++) s_res |= sem_init(&queueSem[i], 0, 0);
    s_res |= sem_init(&accessSem, 0, 1); // Semaphore for accessing resources
    if (s_res) {
        perror("Error:");
        return SEM_ERR;
    }
    int t_res = 0; // Return value from pthread_create calls - error if non-zero
    // Create and start threads
    pthread_t fileRead, ioThread, cpuScheduler;
    void *fileErr;
    t_res |= pthread_create(&fileRead, NULL, fileReadRoutine, &info);
    t_res |= pthread_create(&ioThread, NULL, ioRoutine, &info);
    t_res |= pthread_create(&cpuScheduler, NULL, cpuRoutine, &info);
    pthread_join(fileRead, &fileErr);
    pthread_join(cpuScheduler, NULL);
    pthread_join(ioThread, NULL);
    for (int i = 0; i < 2; i++) {  // Clean up semaphores
        sem_destroy(&queueSem[i]);
    }
    sem_destroy(&accessSem);
    if (fileErr == (void *)FILE_ERR) return FILE_ERR;
    // Print performance metrics
    printf("Input File Name\t\t\t: %s\n", info.inputFile);
    if (hasQuantum) {
        printf("CPU Scheduling Alg\t\t: %s quantum: %d\n", 
            algorithms[info.algorithm], info.quantum);
    } else {
        printf("CPU Scheduling Alg\t\t: %s\n", algorithms[info.algorithm]);
    }
    printf("Throughput\t\t\t: %.3f processes / ms\n", info.completed / info.elapsed);
    printf("Avg. Turnaround time\t\t: %.1f ms\n", info.elapsed / info.completed);
    printf("Avg. Waiting time in R queue\t: %.2f ms\n", info.totalWaitTime / info.completed);
    return 0;
}

