#include "queue.h"

/*
 (int) initializeQueue takes a (Queue *) queue and initializes it with a maximum
 size of (long) size.

 On success, QUEUE_OK is returned. If the provided (Queue *) is NULL, an error
 is returned and the queue is not initialized.
*/
int initializeQueue(Queue *queue, long size) {
    // One safety net - if the provided queue is NULL, error out
    if (queue == NULL) {
        fprintf(stderr, "Error: Bad queue handle.\n");
        return BAD_QUEUE_ERR;
    }
    // Configure the queue with a max size of (long) size
    queue->queue = NULL;
    queue->maxSize = size;
    queue->size = 0;
    return QUEUE_OK;
}

/*
 (int) enqueue takes a (Queue *) queue and a (page_t) item to enqueue in the
 queue. This function attempts to insert the (page_t) item at the head of the
 linked list. When an item is enqueue'd, the current size of the queue is
 increased by one. When the size of the queue is at max capacity, a
 QUEUE_FULL_ERR value is returned to signify the queue cannot accept any more
 items.

 Page table entries that are added to the queue are dynamically allocated so
 they can be inserted into DLL doubly linked lists. These dynamically allocated
 files should be free'd when removed from the queue.

 On success, 0 is returned and the (page_t) item is enqueue'd into the queue and
 the current size of the queue is incremented. Otherwise, an error code is
 returned and the queue remains the same as before the function was called.
*/
int enqueue(Queue *queue, page_t item) {
    int res;
    // If we're at size boundary, do not insert
    if (queue->size + 1 > queue->maxSize) {
        return QUEUE_FULL_ERR;
    }
    page_t *value = malloc(sizeof(page_t));
    *value = item;
    res = insertHeadDLL(&queue->queue, value);
    if (!res) queue->size++; // Increase size by 1
    return res;
}

/*
 (page_t) dequeue takes a (Queue *) queue and grabs the item at the tail of the
 queue. Because items are inserted at the head, grabbing the item at the tail
 ensures items are retrieved in FIFO order.

 (page_t) dequeue depends on items in the queue inserted using the enqueue
 function, or something similar. This is because dequeue assumes that items in
 the doubly linked list are allocated dynamically, so they are free'd once
 taken from the list.

 On success, the page_t value retrieved from the queue is returned. Otherwise,
 QUEUE_EMPTY_ERR (-1) is returned.

 NOTE: page_t should be an unsigned data-type, but the -1 cookie should be
 sufficient to identify errors.
*/
page_t dequeue(Queue *queue) {
    void *data;
    page_t ret;
    if (queue->size - 1 < 0) {
        if (VERBOSE) fprintf(stderr, "Error: Queue empty.\n");
        return QUEUE_EMPTY_ERR;
    }
    // Remove item in FIFO order
    data = removeTailDLL(&queue->queue);
    if (data != NULL) {
        ret = *(page_t *)data;
        queue->size--;
        free(data);
        return ret;
    } else {
        fprintf(stderr, "Error: DLL empty.\n");
        return QUEUE_EMPTY_ERR;
    }
}

/*
 (int) find compares a (void *) a and (void *) b value, converts them into
 page_t pages, zeroes out their 3 most significant bits and compares those
 values. This is equivalent to a page table entry's "EqualsTo" method.

 Returns a non-zero value when (void *) a and (void *b) are equal to one
 another. Otherwise zero is returned.
*/
int find(void *a, void *b) { // Function used to search for an integer
    page_t mask = 0;
    for (int i = 0; i < 3; i++) {
        mask |= ((page_t)1 << ((sizeof(page_t) * 8) - 1 - i));
    }
    return ((*(page_t *) a & ~mask) == (*(page_t *) b & ~mask));
}

/*
 (void) updateItem takes a (Queue *) queue and searches through the active
 pages until the (page_t) item is found. This is used to update activate page
 values in the queue. This might be used when an item is in the queue from a
 previous read access, but a write access marks that page table entry as dirty.
*/
void updateItem(Queue *queue, page_t item) {
    doubleLL **list = &queue->queue;
    if (!list || !(*list)) return; // Check if passed pointers are valid
    doubleLL *current = *list;
    int found = 0;
    while (current) {
        // Check if current node is viable using given search function
        found = find(current->data, &item);
        if (found) {
            // Update item in queue
            *(page_t *)current->data |= item;
            break;
        }
        current = current->next;
    }
}

/*
 (void) refreshQueue takes some (Args *) args containing the queue handle and
 current algorithm to use. This function is the heart of the page replacement
 algorithms used in this program. The provided (page_t) item must be inserted
 into the queue - FIFO/LRU/MIN page eviction policies will be used to determine
 which page must be removed from the queue when space is needed.

 The name REFRESH in the name of the function comes from the fact that under LRU
 when an item is hit, it is refreshed in the queue by being placed as the newest
 head. For other algorithms, the queue is refreshed by evicting old pages and
 inserting new ones when needed.
*/
void refreshQueue(Args *args, page_t item) {
    // Grab algorithm and queue handle from (Args *) args
    Algorithms algorithm = args->algorithm;
    Queue *queue = args->queue;
    // mBit is used to check if a page table entry has a dirty bit
    int res = 0, mBit = 0; // Return code from enqueue
    page_t ret; // Return value from dequeue call
    page_t *queryPage = NULL; // Page retrieved from manual DLL calls
    // Increase the number of references
    pageReferences++;
    // Check if the element exists in the queue
    int exists = containsDLL(&queue->queue, &item, find);
    if (exists) { // If element was removed, put it back
        if (algorithm == LRU) {
            // Remove and refresh item in queue
            queryPage = getDLL(&queue->queue, &item, find);
            if (queryPage) {
                queue->size--;
                // Preserve 3 MSBs
                *queryPage |= item;
                enqueue(queue, *queryPage);
                free(queryPage);
            }
        }
        // Update page currently sitting in queue
        updateItem(queue, item);
    } else {
        // Increase the number of misses
        pageMisses++;
        missTime += 5; // Add 5 time uints to missTime
        // If the item did not exist in the queue already...
        // Check if the queue can accept one more entry
        if (queue->size + 1 > queue->maxSize) {
            switch (algorithm) {
                case LRU:
                case FIFO: // LRU and FIFO share the same eviction policy
                    // Remove the oldest item and add our current item
                    ret = dequeue(queue);
                    break;
                case MIN: // Use the evict min method for MIN
                    ret = evictMinQueue(queue, args->rwList);
                    if (ret == -1) {
                        fprintf(stderr, "Error: Queue min extract error.\n");
                        return;
                    }
                    break;
                default:
                    fprintf(stderr, "Error: Unsupported algorithm.\n");
                    break;
            }
            // Check the dirty bit of the returned page
            mBit = ret & ((page_t)1 << ((sizeof(page_t) * 8) - 2));
            // If the page has been modified, add 10 time units
            if (mBit) writeTime += 10;
        }
        // Add new item into the queue
        res = enqueue(queue, item);
        if (res && VERBOSE) {
            fprintf(stderr, "Error: Unable to add to queue with status %d.\n", res);
        }
    }
}

/*
 (void) destroyQueue cleans up dynamically allocated memory associated with the
 (Queue *) queue and its elements. When "dirty" pages are removed from the queue
 at the end, the writeTime variable is changed to account for the dirty writes
 after the simulation has taken place.
*/
void destroyQueue(Queue *queue) {
    long currentSize = queue->size;
    int mBit;
    for (long i = 0; i < currentSize; i++) {
        page_t item = dequeue(queue);
        // Check the write bit for writebacks at end
        mBit = item & ((page_t)1 << ((sizeof(page_t) * 8) - 2));
        // If the page has been modified, add 10 time units
        if (mBit) writeTime += 10;
    }
}

// a - command, b - long
/*
 (int) rwComFind takes a (void *) a in the form of a (Command *) aC and compares
 it to the value stored in (void *) b, which is a (page_t *) type. The page_t
 value pointed to by (void *) b has its most significant 3 bits masked to zero
 to extract the page number from the page_t modified by special bits.

 Returns a non-zero value if the page number in (void *) a and (void *) b are
 equal to one another. Otherwise a zero is returned.
*/
int rwComFind(void *a, void *b) {
    page_t mask = 0;
    // Build mask to zero out 3 most significant bits
    for (int i = 0; i < 3; i++) {
        mask |= ((page_t)1 << ((sizeof(page_t) * 8) - 1 - i));
    }
    Command *aC = (Command *)a;
    page_t aP = aC->pageNumber;
    // Mask out 3 most significant bits to get raw page number
    page_t bP = *(page_t *)b & ~mask;
    return aP == bP;
}

/*
 (page_t) evictMinQueue takes a (Queue *) queue containing active pages and
 evicts the page that is the farthest away in the (doubleLL **) rwList
 containing the list of (Command *) commands that will be run next.

 This function is used mainly for the MIN algorithm.

 On success, returns the evicted page_t value and the page with an access the
 farthest out is evicted from the provided queue. Otherwise, -1 is returned and
 the queue remains untouched.
*/
page_t evictMinQueue(Queue *queue, doubleLL **rwList) {
    if (queue == NULL || queue->queue == NULL) return -1;
    doubleLL *current = queue->queue;
    page_t page = -1;
    int maxDistance = 0, distance = 0;
    doubleLL *maxPage = NULL;
    // While loop will always iterate at least once
    while (current) {
        page = *(page_t *)current->data;
        distance = pathToDLL(rwList, &page, rwComFind);
        if (distance == -1) {
            maxDistance = -1;
            maxPage = current;
        } else if (distance > maxDistance && maxDistance != -1) {
            maxDistance = distance;
            maxPage = current;
        }
        current = current->next;
    }
    // Remove maxPage if not NULL
    if (maxPage) {
        page = *(page_t *)maxPage->data;
        // Free the allocated page_t in the DLL
        free(maxPage->data);
        // If maxPage is the head of the queue...
        if (maxPage == queue->queue) {
            queue->queue = maxPage->next; // Update head
        }
        // Remove the maxPage from the queue
        removeDLL(maxPage);
        queue->size--;
        // If size is now 0, set queue->queue to NULL
        if (!queue->size) queue->queue = NULL;
    }
    return page;
}
