#ifndef BACKITUP_H
#define BACKITUP_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

/*
 BACKUP and RESTORE macros used to determine the currrent operating mode. See
 documentation for initializeBackup().
*/
#define BACKUP 1
#define RESTORE 2

/*
Struct definitions
*/

/*
 struct threadArgs is a struct that defines the arguments being passed into the
 archive function when an archive task is created. The following members are
 defined as the following:

 * (unsigned int) threadID - The current thread's ID that will be used when
 printing status messages to stdout.
 * (char *) original, backup - Paths that point to the original and backup files
 respectively.
 * (int) mode - Defines the mode that the thread is currently operating in.
*/
typedef struct threadArgs {
    unsigned int threadID;
    char *original, *backup;
    int mode;
} threadArgs;

/*
 struct threadList is a simple linked list interface for creating many threads
 and being able to easily join them. The following members are defined as the
 following:

 * (struct threadList *) next - The next thread "node" in the linked list.
 * (pthread_t) thread - The thread handle for the thread. Need a reference in
 the list when joining on all.
 * (unsigned int) threadID - The threadID of the thread to be printed out during
 activity messages.
*/
typedef struct threadList {
    struct threadList *next;
    pthread_t thread;
    unsigned int threadID;
} threadList;

/*
 The following function prototypes are functions that exist within the
 BackItUp.c source file.
*/
int backup(char *);
int initializeBackup(int);
char *getFileName(char *);
void *archive(void *);
int joinAll(threadList *); // Should only be called at end of entire program
int setTask(threadList **, void *task(void *), char *, char *);
unsigned char fileCompare(char *, char *);
unsigned char getStats(char *, struct stat *);

#endif