#include "doublell.h"

/*
 (int) insertHeadDLL takes a (doubleLL **) list to operate on, and some
 (void *) data to insert as the new head of the list. Unless an error occurs,
 *list will be updated to point to the newly created DLL node. If *list was not
 previously null, a new head is created and the previous head is set as the next
 node in the list. 

 On success, 0 is returned. Otherwise DLL_FAIL is returned. 
*/
int insertHeadDLL(doubleLL **list, void *data) {
    doubleLL *oldHead = *list;
    // Check if list is NULL, create new list
    if (!(*list)) {
        *list = createDLL(data, NULL, NULL);
        return (*list ? 0 : DLL_FAIL);
    }

    // Insert a new entry at head of list
    doubleLL *newHead = createDLL(data, oldHead, NULL);
    if (oldHead) oldHead->prev = newHead; // Update previous pointer
    *list = newHead; // Update current head of list to newHead
    return (*list ? 0 : DLL_FAIL);
}

/*
 (int) insertTailDLL takes a (doubleLL **) list and some (void *) data and
 creates and inserts a new DLL with the given data at the tail of the list. If
 the value of *list is NULL, a new list is created and *list will point to the
 new list head. 

 On success, 0 is returned. Otherwise DLL_FAIL is returned. 
*/
int insertTailDLL(doubleLL **list, void *data) {
    if (!(*list)) { // Create new list when empty
        *list = createDLL(data, NULL, NULL);
        return (*list ? 0 : DLL_FAIL);
    }
    doubleLL *current = *list;
    while (current->next) current = current->next;

    // Configure new tail node
    doubleLL *newTail = createDLL(data, NULL, current);
    current->next = newTail;
    return (newTail ? 0 : DLL_FAIL);

}

/*
 (doubleLL *) createDLL takes some (void *) data, a (doubleLL *) next and 
 (doubleLL *) prev pointer and creates a new DLL node with the given links. 

 On success, a pointer is returned to the newly created DLL node. Otherwise,
 NULL is returned and an error is printed to stderr.
*/
doubleLL *createDLL(void *data, doubleLL *next, doubleLL *prev) {
    doubleLL *newList = malloc(sizeof(doubleLL));
    if (!newList) {
        perror("Error:");
        return NULL;
    }
    newList->next = next;
    newList->prev = prev;
    newList->data = data;

    return newList;
}

/*
 (void *) removeHeadDLL takes a (doubleLL **) list, removes the head node and
 returns the data associated with that node. The dynamically allocated data for
 the removed node is automatically cleared. The value of *list is updated to
 point to the next value in the list.

 On success, a pointer to the removed node's data is returned. Otherwise, NULL
 is returned. 
*/
void *removeHeadDLL(doubleLL **list) {
    if (!(*list)) return NULL;

    doubleLL *oldHead = *list;
    void *data = oldHead->data;
    *list = oldHead->next; // Update list pointer to next element
    // Remove head of list
    removeDLL(oldHead);

    return data; // Return data from the head
}

/*
 (void *) removeTailDLL takes a (doubleLL **) list, removes the DLL node at the
 tail and returns the (void *) data associated with the tail node. If the tail 
 node is the head node, the value of *list is changed to NULL. Dynamically
 allocated memory for removed nodes is automatically cleared when this function
 is called. 

 On success, a pointer to the data from the removed DLL node is returned.
 Otherwise NULL is returned. 
*/

void *removeTailDLL(doubleLL **list) {
    if (!(*list)) return NULL;
    doubleLL *current = *list;
    while (current->next) current = current->next;
    void *data = current->data;
    if (*list == current) *list = NULL; // If head removed, set list to NULL
    removeDLL(current); // Remove current node 
    return data;
}

/*
 (void) removeDLL takes a (doubleLL *) list node, reconfigures links if need be,
 and frees the DLL node. 
*/
void removeDLL(doubleLL *list) {
    if (!list) return;
    if (list->prev) list->prev->next = list->next;
    if (list->next) list->next->prev = list->prev;
    free(list);
}

/*
 (void) cleanDLL takes a (doubleLL **) list and cleans all of the dynamically
 allocated memory created by DLL functions. After clearing the doubly linked
 list, the value of *list is changed to NULL to signify an empty list.
*/
void cleanDLL(doubleLL **list) {
    if (!list || !(*list)) return;
    // Pop elements off one by one to clean list
    while (*list) removeTailDLL(list);
    *list = NULL; // Set list as NULL
}

/*
 (void *) getDLL takes a (doubleLL **) list and searches for a given
 (void *) target node using a given (int) search (void *, void *) function. The
 search function provided is used to search through the nodes in the list and
 return the first one to match the seach query. Dynamically allocated memory for
 removed DLL nodes is automatically cleared after removal. 

 On success, (void *) getDLL returns a pointer to the data associated with a
 removed DLL node. Otherwise, NULL is returned.
*/
void *getDLL(doubleLL **list, void *target, int search (void *, void *)) {
    if (!list || !(*list)) return NULL; // Check if passed pointers are valid
    doubleLL *current = *list;
    void *data = NULL;
    int found = 0;
    while (current) {
        // Check if current node is viable using given search function
        found = search(current->data, target);
        if (found) break;
        current = current->next;
    }
    if (found) {
        data = current->data;
        if (current == *list) {
            *list = current->next; // Update head
        }
        removeDLL(current);
    }
    return data;
}

/*
 (void *) getRankedDLL takes a (doubleLL **) list and grabs the best ranked
 item in the list according to the given (int) search (void *, void *)
 comparison function. This function can be used to retrieve max/min values
 amongst the data in the list. Dynamically allocated memory for removed DLL
 nodes is automatically cleared after removal. 

 On success, (void *) getRankedDLL returns the data associated with the DLL node
 that has the best ranking among the other nodes in the list. Otherwise NULL
 is returned.  
*/
void *getRankedDLL(doubleLL **list, int search (void *, void *)) {
    if (!list || !(*list)) return NULL; // Check if passed pointers are valid
    doubleLL *current = *list, *ideal = NULL;
    void *data = NULL;
    int rank = 0;
    ideal = current;
    while (current->next) {
        current = current->next;

        // Compare the previous and current node data
        rank = search(current->data, ideal->data);
        if (rank > 0) { // If the rank is better for current, update ideal node
            ideal = current;
        }
    }
    if (!ideal->prev) *list = ideal->next; // Update head
    data = ideal->data; // Set data to return ideal's data value
    removeDLL(ideal);
    return data; // Return highest ranked data
}

/*
 (void) printListDLL takes a (doubleLL *) list and prints out each node's data
 value and who's next node they point to in the list. Because the (void *) data
 is abstracted, an (int) unwrap (void *) function is required to parse data into
 an integer form.
*/
void printListDLL(doubleLL *list, int unwrap (void *)) {
    if (!list) return;
    doubleLL *current = list;
    while (current) {
        printf("Node [%d] | prev [%d] next[%d]\n",
            unwrap(current->data),
            current->prev ? unwrap(current->prev->data) : -1,
            current->next ? unwrap(current->next->data) : -1);
        current = current->next;
    }
}
