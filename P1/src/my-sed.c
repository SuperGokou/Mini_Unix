#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
(int) processInput looks for the substring (char *) find in the file at path
(char *) filename and replaces it with the (char *) replace string. If the given
(char *) filename argument is NULL, (int) processInput will read input from
stdin.

(int) processInput returns 0 on success. If the file at (char *) filename cannot
be opened, 1 is returned. 
*/
int processInput(char *find, char *replace, char *filename) {
    FILE *fp = NULL;
    char *lineptr = NULL;
    size_t size = 0;
    ssize_t readChars = 0;
    int target = strlen(find); // how many characters are in target word
    if (filename) {
        fp = fopen(filename, "r");
        if (!fp) return 1;
    }
    while (readChars != -1) {
        if (!filename) readChars = getline(&lineptr, &size, stdin);
        else readChars = getline(&lineptr, &size, fp);
        int count = 0; // When count = target, replace that substring
        int startReplace = -1; // index where replaceable word is found
        for (int i = 0; i < readChars; i++) { // Iterate through characters in word
            if (lineptr[i] == find[count]) {
                count++;
                if (startReplace == -1) startReplace = i;
            } else {
                count = 0;
                startReplace = -1;
                // Re-evaluate last character if it matches
                if (lineptr[i] == find[count]) i--;
            }
            if (count == target) break;
        }
        for (int i = 0; i < readChars; i++) {
            if (i == startReplace) {
                for (int j = 0; j < strlen(replace); j++) {
                    fputc(replace[j], stdout);
                }
                i += target - 1; // Shift i forward in loop
            } else {
                fputc(lineptr[i], stdout);
            }
        }
        free(lineptr);
        size = 0;
        lineptr = NULL;
    }
    if (lineptr) free(lineptr); // lineptr should always be freed after getline()
    if (fp) fclose(fp); // Close open file
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("my-sed: find term replace term [file ...]\n");
        return 1;
    }
    char *find = argv[1], *replace = argv[2];
    int returnCode = 0;
    if (argc == 3) {
        returnCode = processInput(find, replace, NULL);
    } else {
        for (int i = 3; i < argc; i++) {
            returnCode = processInput(find, replace, argv[i]);
            if (returnCode == 1) {
                printf("my-sed: cannot open file\n");
                return 1;
            }
        }
    }
    return 0;
}
