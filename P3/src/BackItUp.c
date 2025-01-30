#include "BackItUp.h"

// Globals for BackItUp operations
static const char *backupPrefix = ".backup/";
static const char *backupSuffix = ".bak";
static const char *ignore[3] = { ".", "..", ".backup" };
static int mode;
static int copies;
static int bytes;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// List of threads to operate on
static threadList *threads = NULL;

/*
 (char *)getFileName takes a (char *) path and returns a pointer to the
 character immediately after the final slash '/' character is encountered. This
 function is used primarily for printing the names of files being backed up.

 On success, (char *) getFileName returns a pointer to the first character of
 the name of the file. Otherwise, NULL is returned. 
*/
char *getFileName(char *path) {
    if (path == NULL) return NULL;
    char *current = path;
    char *nameStart = path;
    while (*current != 0) {
        if (*current == '/') nameStart = ++current;
        else current++;
    }
    if (nameStart == NULL) return NULL;
    return nameStart; 
}

/*
 (int) initializeBackup takes a (int) modeSet from its called and sets the
 current operating mode to that value. The mode for backup operations is defined
 as the following:

 * mode == BACKUP - Backup all of the files in the current working directory.
 * mode == RESTORE - Restore all of the files from the backup directory to the
 current working directory.

 When backing up files, files that are older than their backups are ignored.
 Likewise, when restoring files, files that are newer than their backups are not
 restored. 

 On success, (int) initializeBackup returns 0 and the files in the current
 working diredtory are backed up. Otherwise, an error code is returned and an
 indeterminate number of files in the current working directory are backed up.
*/
int initializeBackup(int modeSet) {
    int ret = 0; // Return code
    char *backupPath = ".backup/";
    mode = modeSet;
    // Create backup folder if it does not already exist
    int res = mkdir(backupPath, 0755);
    if (res == -1 && errno != EEXIST) {
        // Error if mkdir failed not due to it already existing
        perror("Directory Error");
        return -1;
    }
    pthread_mutex_init(&mutex, NULL); // Initialize mutex for copy stats
    ret |= backup(".");
    ret |= joinAll(threads);
    // Display the number of copies copied
    printf("Successfully copied %d files (%d bytes)\n", copies, bytes);
    // Destroy mutex after program is finished
    pthread_mutex_destroy(&mutex);
    return ret;
}

/*
 (int) backup takes a (char *) path that points to a directory and backs up each
 file and subdirectory. (int) backup is called recursively and should only run
 sequentially in the main process. This function specifically creates "tasks"
 (see definition of a task in setTask() documentation) that handle archiving
 files. Functionality can differ slightly depending on the value of (int) mode.

 On success, backup returns 0 and "tasks" are created to archive files
 independently. Otherwise, an error code is returned and an indeterminate number
 of tasks are created.
*/
int backup(char *path) {
    // Create the backup and search paths
    char backupPath[strlen(path) + strlen(backupPrefix) + 2];
    char searchPath[strlen(path) + 2];
    // Zero out strings to ensure free space acts as null-terminator
    memset(backupPath, 0, sizeof(backupPath));
    memset(searchPath, 0, sizeof(searchPath));
    // Build backup path by performing string concatinations
    strcat(backupPath, backupPrefix);
    strcat(backupPath, path);
    strcat(backupPath, "/");
    // Build search path
    strcat(searchPath, path);
    strcat(searchPath, "/");
    // Open search directory or backup directory depending on (int) mode.
    DIR *currentDir;
    if (mode == BACKUP) currentDir = opendir(searchPath);
    else currentDir = opendir(backupPath);
    // Iterate through all records in the current directory
    struct dirent *record;
    while ((record = readdir(currentDir)) != NULL) {
        // If the record is for a regular file, archive it
        if (record->d_type == DT_REG) {
            // In restore mode,  IGNORE executable (it's busy right now...)
            if (mode == RESTORE) {
                if (strcmp(record->d_name, "BackItUp.bak") == 0) continue;
            }
            // Allocate strings dynamically so they can live in threads
            char *searchFile = calloc(strlen(searchPath) + 
                strlen(record->d_name) + 1, sizeof(char));
            char *backupFile = calloc(strlen(backupPath) +
                strlen(record->d_name) + strlen(backupSuffix) + 1, 
                sizeof(char));
            // Build regular file path
            strcat(searchFile, searchPath);
            if (mode == RESTORE) { // Truncate .bak if restoring
                strncat(searchFile, record->d_name, strlen(record->d_name) - 4);
            } else {
                strcat(searchFile, record->d_name);
            }
            // Build backup file path
            strcat(backupFile, backupPath);
            strcat(backupFile, record->d_name);
            if (mode == BACKUP) { // Only add .bak if we're in BACKUP mode
                strcat(backupFile, backupSuffix);
            }
            // Create new thread for two files
            setTask(&threads, archive, searchFile, backupFile);
        // If the record is a directory, recursively inspect it
        } else if (record->d_type == DT_DIR) {
            int pass = 0; // Flag determines if we should ignore directory
            // Check if the directory should be ignored
            for (int i = 0; i < (sizeof(ignore) / sizeof(char *)); i++) {
                // ignore array contains names of directory entries to ignore
                if (strcmp(ignore[i], record->d_name) == 0) pass = 1;
            }
            if (!pass) {
                // Make directory and recurse
                int subPathLen = strlen(searchPath) + strlen(record->d_name) + 2;
                char subPath[subPathLen];
                char backSubPath[strlen(backupPath) + strlen(record->d_name) + 1];
                // Zero out subPath string for null-terminators
                memset(subPath, 0, sizeof(subPath));
                memset(backSubPath, 0, sizeof(backSubPath));
                // Build subPath string to recurse into
                strcat(subPath, searchPath);
                strcat(subPath, record->d_name);
                strcat(backSubPath, backupPath);
                strcat(backSubPath, record->d_name);
                // Make the backup directory if it does not already exist
                int res;
                if (mode == BACKUP) res = mkdir(backSubPath, 0755);
                else res = mkdir(subPath, 0755);
                if (res == -1 && errno != EEXIST) {
                    // Error if mkdir failed not due to it already existing
                    perror("Directory Error");
                    return -1;
                }
                backup(subPath);
            }
        }
    }
    if (closedir(currentDir) == -1) {
        perror("Close Directory Error");
    }
    return 0;
}

/*
 (int) copy takes a (char *) src source file and a (char *) dest destination
 and copies the contents from the source file into the destionation file.

 On success, the file is copied over and the number of bytes copied is returned.
 Otherwise, an error code is returned and the file may not be completely copied
 over, if at all.
*/
int copy(char *src, char *dest) {
    int buffsize = 256;
    char buffer[buffsize];
    unsigned int size = 0;
    int destfd = open(dest, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0644);
    if (destfd == -1) {
        printf("File err [%s]\n", dest);
        perror("Error BackFD");
        size = -1; 
    }
    int srcfd = open(src, O_RDONLY);
    if (srcfd == -1) {
        perror("Error OriginalFD back");
        size = -1;
    }
    int readBytes = read(srcfd, buffer, buffsize);
    int bytesWritten = 0;
    while (readBytes > 0) {
        bytesWritten = write(destfd, buffer, readBytes);
        if (bytesWritten == -1) {
            perror("Write Error");
            size = -1; // Set error code
            break;
        }
        size += readBytes;
        readBytes = read(srcfd, buffer, buffsize);
    }
    if (readBytes == -1) {
        perror("Read Error");
        size = -1; // Set error code
    }
    close(destfd);
    close(srcfd);
    if (size != -1) {
        pthread_mutex_lock(&mutex);
        copies++;
        bytes += size;
        pthread_mutex_unlock(&mutex);
    }
    return size;
}

/*
 (void *) archive takes a (void *) args reference to a dynamically allocated
 threadArgs instance and archives a file. Depending on the (int) mode, the
 original file is either backed up or restored. Detailed operations of each
 mode are specified below:

 BACKUP - Original file is backed up if the original file is newer than the 
 backed up version, or if the backed up version does not exist.

 RESTORE - Original file is restored using the backed up version of the file. If
 the backed up version is newer, or the original file no longer exists, the file
 is restored. Otherwise the file is not restored.

 On success, (void *) archive returns 0 and backup/restore operations are 
 complete. Otherwise, an error code is returned and an indeterminate number of 
 files are backed up/restored.
*/

void *archive(void *args) {
    threadArgs *info = (threadArgs *)args;
    unsigned char res = fileCompare(info->original, info->backup);
    long ret = 0;
    char *originalFile = getFileName(info->original);
    char *backupFile = getFileName(info->backup);
    unsigned int size = 0;
    if (info->mode == BACKUP) {
        // If the original is newer than the backup... backup
        if (res & (1<<1)) {
            printf("[thread %d] Backing up %s\n", info->threadID, originalFile);
            // If we're overwriting an old backup, give warning
            printf("[thread %d] WARNING: Overwriting %s\n", 
                info->threadID, backupFile);
            size = copy(info->original, info->backup); // Create backup
            printf("[thread %d] Copied %u bytes from %s to %s\n", 
                info->threadID, size, originalFile, backupFile);
        }
        // If the backup exists, do NOT touch
        else if (res & (1<<4)) {
            printf("[thread %d] %s does not need backing up\n", 
                info->threadID, originalFile);
        } else {
            printf("[thread %d] Backing up %s\n", info->threadID, originalFile);
            size = copy(info->original, info->backup); // Create backup
            printf("[thread %d] Copied %u bytes from %s to %s\n", 
                info->threadID, size, originalFile, backupFile);
        }
    }
    if (info->mode == RESTORE) {
        // If the backup file is newer, restore.
        // If the original does not exist, but we have a backup, restore
        if (res & (1<<0) || !(res & (1<<5))) {
            size = copy(info->backup, info->original);
            printf("[thread %d] Copied %u bytes from %s to %s\n", 
                info->threadID, size, backupFile, originalFile);
        }
        // If the original file exists, we do not touch
        else {
            printf("[thread %d] %s is already the most current version\n", 
                info->threadID, originalFile);
        }
        
    }
    free(info->backup);
    free(info->original);
    free(args);
    // Configure the return code if an error occured.
    ret = size == -1 ? -1 : 0;
    return (void *)ret;
}

/*
 (unsigned char) fileCompare takes a (char *) src source file and compares its
 metadata to the (char *) dest destination file. The returned 8 bit number
 contains the results of the comparison. Each bit, starting from the LSB
 (least significant bit) contains relevant information about the files, as
 described below:

 bit 0 (LSB) - dest is newer.
 bit 1 - src is newer.
 bit 2 - dest is a directory.
 bit 3 - src is a directory.
 bit 4 - dest exists.
 bit 5 - src exists.

 An example return value from this function would be 110010 -> 0x32, meaning
 that both src and dest exist, both are regular files and src is the newer file.
*/
unsigned char fileCompare(char *src, char *dest) {
    struct stat srcInfo;
    struct stat destInfo;
    unsigned char info = 0;
    // Get stats for src file
    info = getStats(src, &srcInfo);
    // Layer bits from dest file by shifting bits to the right one
    info |= (getStats(dest, &destInfo)>>1);
    if (info & (1<<4) && info & (1<<5)) {
        // Compare the modification dates on the two files to determine final bits
        if (srcInfo.st_mtim.tv_sec == destInfo.st_mtim.tv_sec) {
            if (srcInfo.st_mtim.tv_nsec == destInfo.st_mtim.tv_nsec) {
                // If the modification dates are the same, last 2 bits are set
                info |= (1<<0); // Set LSB (bit 0) to 1
                info |= (1<<1); // Set bit 1 to 1
            } else {
                // Files differ by the nano second
                info |= srcInfo.st_mtim.tv_nsec > destInfo.st_mtim.tv_nsec ?
                    (1<<1) : (1<<0);
            }
        } else {
            // Files differ by a second or more
            info |= srcInfo.st_mtim.tv_sec > destInfo.st_mtim.tv_sec ?
                (1<<1) : (1<<0);
        }
    }
    
    return info;
}

/*
 (unsigned char) getStats is a helper function to the fileCompare function that
 takes a (char *) path and a (struct stat *) stats that points to a stat struct
 belonging to fileCompare and loads it with information using stat(). 

 Returns an unsigned char with bits set for the src file as described in the
 fileCompare function description. 
*/
unsigned char getStats(char *path, struct stat *stats) {
    unsigned char info = 0;
    if (stat(path, stats) == 0) {
        // Set directory bit
        info |= stats->st_mode == S_IFDIR ? (1<<3) : 0;
        // Set existence bit
        info |= (1<<5);
    }
    return info;
}

/*
 (int) setTask takes a (threadList **) list and a (void *task(void *)) function
 and adds a new thread running that function to the threadList. To create a
 list, if (threadList **) list points to a NULL pointer, the list pointer is
 changed to the head of the list. Subsequent calls with this 
 (threadList **) list function add new tasks to the tail of the list.

 A task, in this case, is defined as a thread that runs the given
 (void *task(void *)) pointer. 

 The passed (char *) src and (char *) dest functons are used to build the
 threadArgs struct associated with each task. (char *) src in this case is the
 original file, and (char *) dest is the backed up file path. 

 On success, a new thread is added to the list and 0 is returned. Otherwise,
 -1 is returned and an error message is printed to stderr.
*/
int setTask(threadList **list, void *task(void *), char *src, char *dest) {
    static unsigned int threadCount; // BSS initialized to zero, NO MATTER WHAT!
    threadArgs *args;
    if (list == NULL) {
        fprintf(stderr, "Error: Invalid list pointer\n");
        return -1;
    }
    // Check if the list is NULL, if so, create a "new" one
    if (*list == NULL) {
        *list = malloc(sizeof(threadList));
        if (*list == NULL) {
            perror("Error");
            return -1;
        }
        (*list)->next = NULL;
        // Create the task starting with thread ID 1
        (*list)->threadID = ++threadCount;
        args = malloc(sizeof(threadArgs));
        if (args == NULL) {
            perror("Error");
            return -1;
        }
        args->threadID = (*list)->threadID;
        args->original = src;
        args->backup = dest;
        args->mode = mode;
        int res = pthread_create(&(*list)->thread, NULL, task, args);
        if (res) fprintf(stderr, "Error: %s\n", strerror(res));
    } else {
        // Walk to the end of the list and create a new thread there
        threadList *current = *list;
        while (current->next != NULL) current = current->next;
        current->next = malloc(sizeof(threadList));
        current = current->next;
        current->threadID = ++threadCount;
        current->next = NULL;
        args = malloc(sizeof(threadArgs));
        if (args == NULL) {
            perror("Error");
            return -1;
        }
        args->threadID = current->threadID;
        args->original = src;
        args->backup = dest;
        args->mode = mode;
        int res = pthread_create(&(current->thread), NULL, task, args);
        if (res) fprintf(stderr, "Error: %s\n", strerror(res));
    }
    return 0;
}

/*
 (int) joinAll takes a (threadList *) list of threads and joins them all
 together. Dynamically allocated memory is free'd whenever a thread is joined.

 On success, 0 is returned. Otherwise, an error code is returned and program
 behavior is undefined. 
*/
int joinAll(threadList *list) {
    // ret - return code. res - used for pthread return value.
    long ret = 0;
    void *res;
    if (list == NULL) {
        return -1;
    }
    threadList *current = list, *prev = NULL;
    while (current != NULL) {
        pthread_join(current->thread, &res);
        ret |= (long)res;
        prev = current;
        current = current->next;
        free(prev);
    }
    return ret;
}
