#ifndef DOUBLELL_H
#define DOUBLELL_H

#include <stdlib.h>
#include <stdio.h>

#define DLL_FAIL 2
#define DLL_PATH_FAIL -2
#define DLL_EMPTY_FAIL -1

/*
 doubleLL is a structure used to create doubly linked lists. Definitions for the
 members of the struct are below:
 * (struct doubleLL *) prev - Points to the previous node in the linked list.
 * (struct doubleLL *) next - Points to the next node in the linked list.
 * (void *) data - The data contained in each node in the linked list.
*/
typedef struct doubleLL {
    struct doubleLL *prev;
    struct doubleLL *next;
    void *data;
} doubleLL;

/*
 Below are operations used for performing operations on the doubly linked list.
 Please check comment blocks in doublell.c for information on each function.
*/
int insertHeadDLL(doubleLL **list, void *data);
int insertTailDLL(doubleLL **list, void *data);
doubleLL *createDLL(void *data, doubleLL *next, doubleLL *prev);
void *removeHeadDLL(doubleLL **list);
void *removeTailDLL(doubleLL **list);
void removeDLL(doubleLL *list);
void cleanDLL(doubleLL **list);
void printListDLL(doubleLL *list, int unwrap (void *));

// Flexible DLL search functions
void *getDLL(doubleLL **list, void *target, int search (void *, void *));
void *getRankedDLL(doubleLL **list, int search (void *, void *));

// Check if contains
int containsDLL(doubleLL **list, void *target, int search (void *, void *));
int pathToDLL(doubleLL **list, void *target, int search (void *, void *));

#endif
