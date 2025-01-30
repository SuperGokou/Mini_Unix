#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {
    int buffsize = 128;
    char buffer[buffsize]; // Buffer for reading file contents
    // Check for proper command line arguments
    if (argc < 2) {
        return 0;
    }
    // Can open multiple files at once
    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r"); // Open passed file in read-only mode
        if (fp == NULL) { // Check if fopen failed
            printf("my-cat: cannot open file\n");
            return 1;
        }
        while (1) {
            char *result = fgets(buffer, buffsize, fp);
            if (ferror(fp)) {
                perror("Error");
                return 2;
            } else if (result == NULL) {
                break; // Break the infinite loop after writing file contents
            }
            printf("%s", result);
        }
        if (fclose(fp)) { // Close the file when finished
            perror("Error");
            return 2;
        } 
    }
    
    return 0; // Should always have exit code 0 after normal run
}