#ifndef QUEUE_H
#define QUEUE_H

#include "doublell.h"

// Return codes for Queue operation errors
#define BAD_QUEUE_ERR 1
#define QUEUE_FULL_ERR 3
#define QUEUE_OK 0
#define QUEUE_EMPTY_ERR -1

// Setting VERBOSE to 1 will display extra error messages
#define VERBOSE 0

// List of supported algorithms for page replacement simulation
typedef enum Algorithms {
    FIFO, LRU, MIN
} Algorithms;

// Create page_t type for pages - can change size whenever desired
typedef unsigned int page_t;

/*
 The Queue struct defines the handle used for maintaining the queue data
 structure for page simulation. Members of this struct are the following:
 * (doubleLL *) queue - The internal doubleLL doubly linked list that is used to
   maintain the queue. This holds the actual data if you will.
 * (long) size - The current size of the queue. When 0, the queue is empty. If
   dequeue is called and the current size is 0, the queue function will return a
   QUEUE_EMPTY_ERR. If size = maxSize and enqueue is called, the queue function
   will return a QUEUE_FULL_ERR.
 * (long) maxSize - The maximum size of the queue.
*/
typedef struct Queue {
    doubleLL *queue;
    long size;
    long maxSize;
} Queue;

/*
 The Args struct is a struct that contains command line arguments and references
 to data structures used across several functions. Members of this struct are
 defined as the following:
 * (Algorithms) algorithm - An (Algorithms) enumerator instance that contains
   the current algorithm used in the simulation. This algorithm is provided
   through the command line when invoking the program.
 * (long) cacheSize - The number of active pages that can be present in the
   active page structure.
 * (char *) inputFile - The path to the input file to process to run the page
   replacement algorithm on.
 * (Queue *) queue - A reference to a queue structure to be used in the page
   replacement algorithm.
 * (doubleLL **) rwList - A list containing the read/write operations retrieved
   from the input file.
*/
typedef struct Args {
    Algorithms algorithm;
    long cacheSize;
    char *inputFile;
    Queue *queue;
    doubleLL **rwList;
} Args;

// Enumerator for supported Modes - READ/WRITE
typedef enum Mode {
    READ, WRITE
} Mode;

/*
 The Command struct contains a list of "commands" to perform in the page
 replacement simulation algorithm. Members of this struct are the following:
 * (Mode) mode - Determines if an access is READ or WRITE.
 * (long) pageNumber - The page number that the command is accessing.
*/
typedef struct Command {
    Mode mode;
    long pageNumber;
} Command;

// Get statistic globals from main.c
extern int pageReferences, pageMisses;
extern int missTime, writeTime;

/*
 Queue operations for page simulation - see function declarations for more
 detailed information.
*/
int initializeQueue(Queue *queue, long size);
int enqueue(Queue *queue, page_t item);
page_t dequeue(Queue *queue);
void refreshQueue(Args *args, page_t item);
void destroyQueue(Queue *queue);
void updateItem(Queue *queue, page_t item);
page_t evictMinQueue(Queue *queue, doubleLL **rwList);

#endif
