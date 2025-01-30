#include "BackItUp.h"

int main(int argc, char **argv) {
    int mode = BACKUP; // Mode set to backup by default
    // Parse arguments and check for optional -r flag
    for (int i = 1; i < argc; i++) {
        // Check command line arguments for -r flag
        if (strncmp(argv[i], "-r", 3) == 0) {
            // Set some variable indicating restore mode
            mode = RESTORE;
        }
    }
    // Perform the backup operations
    int result = initializeBackup(mode);

    return result;
}