#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "queue.h"
#define REQARGS 4 // Required argument count
#define FILE_ERR 5

// Track the number of page references/misses
int pageReferences, pageMisses;
// Keep track of the miss time and dirty write time units
int missTime, writeTime;

/*
 (void) signalInput takes a (Mode) mode (READ/WRITE), (page_t) value to
 access in the page table, and (Args *) args from the command line.

 NOTE: The 3 most significant bits are reserved for special bits associated with
 the page table. The MSB (bit 31) is the valid bit, bit 30 is the dirty bit, and
 bit 29 is an unused reference bit. By default, page_t is an unsigned 32 bit
 integer, but it can be safely changed to an unsigned char, long, etc., within
 reason of course. 

 The generated PTE page is pushed to the refereshQueue() function so that it is
 inserted into the queue-like structure.
*/
void signalInput(Mode mode, page_t value, Args *args) {
    page_t mask = 0;
    page_t PTE = value;
    // Zero out bits 31, 30, and 29 from value
    for (int i = 0; i < 3; i++) {
        value &= ~((page_t)1 << ((sizeof(page_t) * 8) - 1 - i));
        mask |= ((page_t)1 << ((sizeof(page_t) * 8) - 1 - i));
    }
    // Set MSB to 1 - valid bit (bit 31)
    PTE |= ((page_t)1 << ((sizeof(page_t) * 8) - 1));
    // If in write mode, set bit 30 to 1 (dirty bit)
    if (mode == WRITE) PTE |= ((page_t)1 << ((sizeof(page_t) * 8) - 2));
    PTE &= mask; // Clear bits 28-0
    PTE |= (value & ~mask); // Set bits 28-0 (write value to PTE)
    refreshQueue(args, PTE);
}

/*
 (int) processInput takes some (Args *) args from the command line, opens the
 input file and simulates the page replacementa algorithms.

 On success, 0 is returned and the page replacement simulation was run
 successfully. Otherwise, an error code is returned and an error message is
 printed to stderr.
*/
int processInput(Args *args) {
    // Open the input file with read permissions
    FILE *fp = fopen(args->inputFile, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file %s\n", args->inputFile);
        return FILE_ERR;
    }
    char *line = NULL; // Use heap memory for getline
    size_t linelen = 0;
    char *currentToken = NULL;
    // Load reads/writes into a temp linked list
    doubleLL *rwList = NULL;
    int rwListSize = 0; // Size of rwList
    while (getline(&line, &linelen, fp) != -1) {
        currentToken = strtok(line, " "); // Separate by spaces
        // Check what the first token is for this line
        if (!strncmp(currentToken, "R", 1)) {
            currentToken = strtok(NULL, " ");
            long value = strtol(currentToken, NULL, 10);
            // Insert data into DLL
            Command *command = malloc(sizeof(Command));
            command->mode = READ; // Mark command as READ
            command->pageNumber = value;
            if (insertTailDLL(&rwList, command) == DLL_FAIL) {
                fprintf(stderr, "Error: DLL insert error.\n");
            } else {
                // If successful insert into DLL, increase size
                rwListSize++;
            }
        } else if (!strncmp(currentToken, "W", 1)) {
            currentToken = strtok(NULL, " ");
            long value = strtol(currentToken, NULL, 10);
            // Insert data into DLL
            Command *command = malloc(sizeof(Command));
            command->mode = WRITE; // Mark command as WRITE
            command->pageNumber = value;
            if (insertTailDLL(&rwList, command) == DLL_FAIL) {
                fprintf(stderr, "Error: DLL insert error.\n");
            } else {
                // If successful insert into DLL, increase size
                rwListSize++;
            }
        } else {
            fprintf(stderr, "Error: Unknown item %s.\n", currentToken);
            break;
        }
    }
    Command *data;
    args->rwList = &rwList;
    for (int i = 0; i < rwListSize; i++) {
        // Remove list head items and operate on them
        data = (Command *)removeHeadDLL(&rwList);
        if (data->mode == READ) {
            signalInput(READ, data->pageNumber, args);
            // Free the current command's data
        } else if (data->mode == WRITE) {
            signalInput(WRITE, data->pageNumber, args);
        } else {
            fprintf(stderr, "Error: Invalid command.\n");
        }
        free(data);
    }
    free(line); // Free line on heap
    fclose(fp); // Close file after processing
    return 0;
}

int main (int argc, char **argv) {
    Args args;
    int res;
    // Must have 4 args EXACTLY
    if (argc != REQARGS) {
        printf("\nUsage: ./simulate <FIFO | LRU | MIN> <cache size> <input file>\n\n");
        return 1;
    }
    // Parse args - must be in order
    for (int i = 1; i < argc; i++) {
        switch (i) {
            case 1:
                // Set the algorithm to use
                if (strcmp(argv[i], "FIFO") == 0) args.algorithm = FIFO;
                if (strcmp(argv[i], "LRU") == 0) args.algorithm = LRU;
                if (strcmp(argv[i], "MIN") == 0) args.algorithm = MIN;
                break;
            case 2:
                // Set the cache size to use
                args.cacheSize = strtol(argv[i], NULL, 10);
                break;
            case 3:
                // Set the input file to use
                args.inputFile = argv[i];
                break;
            default:
                break;
        }
    }
    // Create queue
    Queue queue;
    res = initializeQueue(&queue, args.cacheSize);
    if (res) {
        fprintf(stderr, "Error: Failed to initialize queue!\n");
        return res;
    }
    // Set queue reference in args to our initialized queue
    args.queue = &queue;
    res = processInput(&args);
    destroyQueue(&queue); // Safely clear memory used by queue
    printf("-----------------------------------------\n");
    // Print out stats
    printf("Algorithm: \t\t");
    switch (args.algorithm) {
        case FIFO:
            printf("FIFO\n");
            break;
        case LRU:
            printf("LRU\n");
            break;
        case MIN:
            printf("MIN\n");
            break;
        default:
            printf("NULL\n");
            break;
    }
    printf("References: \t\t%d\n", pageReferences);
    printf("Page misses: \t\t%d\n", pageMisses);
    printf("Page miss time units: \t%d\n", missTime);
    printf("Dirty write time units: %d\n", writeTime);
    printf("Total time: \t\t%d\n", missTime + writeTime);
    printf("-----------------------------------------\n");
    return res;
}
