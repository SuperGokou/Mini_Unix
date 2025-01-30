#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFSIZE 128
#define READ_FILE 0
#define READ_STDIN 1

/*
The line struct is used to hold complete lines read from a file at a time. The
following struct members are described below:
 * (char *) data - Holds a null-terminated string contained within a single
 line. The (char *) data member is going to be used for comparisons to find
 unique lines in a file.
 * (int) length - The length of the string contained within the line. This
 length value is used to resize the (char *) data section when necessary.
*/
typedef struct line {
    char *data;
    int length;
} line;

line *createLine(void);
int pushLineData(line *, char *, int);

/*
(line *) createLine is called with no arguments and dynamically allocates a new
(struct line) on the heap.

On success, (line *) createLine returns a pointer to a newly created line
structure. On failure, NULL is returned and the error is printed to stderr.
*/
line *createLine() {
    line *newLine = malloc(sizeof(line));
    if (newLine == NULL) {
        perror("Error");
        return NULL;
    }
    newLine->data = NULL;
    return newLine;
}

/*
(int) pushLineData takes a (line *) current line to write data to, some
(char *) data to write to the (line *) current line, and (int) length to write
to the current line. (int) pushLineData will allocate memory on the heap for a
new line, or will resize the data stored in (line *) current to concatenate the
given (char *) data to it.

On success, 0 is returned. On failure, 1 is returned an the error is printed to
stderr.
*/
int pushLineData(line *current, char *data, int length) {
    if (!current->data) {
        // Malloc line data with (char *)data
        current->data = malloc(sizeof(char) * length + 1);
        if (!current->data) {
            perror("Error");
            return 1;
        }
        memcpy(current->data, data, sizeof(char) * length);
        current->data[length] = 0; // Add null-terminator
        current->length = length + 1;
        return 0;
    } else { // Memory has already been allocated for data, so resize
        char *temp = NULL; // Temporary (char *) for resizing
        int originalLength = current->length;
        current->length += length;
        temp = realloc(current->data, sizeof(char) * current->length);
        if (!temp) { // Resize failure
            perror("Error");
            return 2;
        }
        current->data = temp;
        memcpy(&current->data[originalLength - 1], data, sizeof(char) * length);
        current->data[originalLength - 1 + length] = 0; // Re-add original terminator
        return 0;
    }
}
/*
(int) processLineData takes an (int) mode variable describing the function's
operating mode and a (char *) path describing the path to a file to read.
The operating modes that (int) processLineData uses are below:
 * READ_FILE - Function opens a file located at (char *) path and processes
 the line data for that file.
 * READ_STDIN - Function will read directly from STDIN and process line data
 from it.

On success, (int) processLineData will return 0. If (int) processLineData
cannot open the file located at (char *) path, an error message is printed and
1 is returned. For all other errors, 2 is returned an the error is printed to
stderr.
*/
int processLineData(int mode, char *path) {
    char buffer[BUFFSIZE];
    line *previous = NULL, *current = NULL; // Previous and current lines for comparison
    FILE *fp;
    if (mode == READ_FILE) {
        fp = fopen(path, "r"); // Open passed file in read-only mode
        if (!fp) { // Check if fopen failed
            printf("my-uniq: cannot open file\n");
            return 1;
        }
    }
    while (1) {
        char *result;
        switch(mode) {
            case READ_FILE:
                result = fgets(buffer, BUFFSIZE, fp);
                if (ferror(fp)) {
                    perror("Error");
                    return 2;
                }
                break;
            case READ_STDIN:
                result = fgets(buffer, BUFFSIZE, stdin);
                break;
            default:
                fprintf(stderr, "Error: Unknown mode\n");
                return 2;
        }
        if (!result) {
            // Free previous/current lines if they exist
            if (previous) {
                free(previous->data);
                free(previous);
            }
            if (current) {
                free(current->data);
                free(previous);
            }
            break; // Break the infinite loop after writing file contents
        }
        if (!current) {
            current = createLine();
            if (!current) return 2;
        }
        int bufflen = strlen(buffer);
        pushLineData(current, buffer, bufflen);
        if (bufflen == BUFFSIZE - 1 && buffer[BUFFSIZE - 2] != '\n') {
            continue; // Line is not complete, run loop once more
        }
        if (!previous) {
            previous = current;
            printf("%s", previous->data);
            current = NULL;
        } else {
            // Compare current to previous
            int diff = strcmp(current->data, previous->data);
            if (diff) {
                free(previous->data);
                free(previous);
                printf("%s", current->data);
                previous = current;
                current = NULL;
            } else {
                free(current->data);
                free(current);
                current = NULL;
            }
        }
    }
    // Close the open file if we're using READ_FILE mode
    if (mode == READ_FILE) {
        if (fclose(fp)) { // Close the file when finished
            perror("Error");
            return 2;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) { // Read from stdin
        int result = processLineData(READ_STDIN, NULL);
        if (result) return result;
    }
    for (int i = 1; i < argc; i++) { // Loop through file paths
        int result = processLineData(READ_FILE, argv[i]);
        if (result) return result;
    }
    return 0;
}
