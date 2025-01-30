# Mini-Unix-Utilities

## Team

* Christian Galvez
* Ethan Pongon
* Ming Xia

### Compiling

From inside of the project folder, simply run `make` and my-cat, my-sed and my-uniq will all be compiled at once. To clean up binary files in the project folder, run `make clean`. 

### my-cat.c

The my-cat.c program mimics the behavior of the UNIX utility called cat, which displays the contents of a file to stdout. After compiling, the my-cat program takes the following arguments from the command line:

`./my-cat [file ...]`

If no files are supplied, my-cat will just close with an exit code of 0. If my-cat is passed a file it cannot open, the error message "my-cat: cannot open file" is printed to stdout and 1 is returned. If any other errors occur, the global errno value is used to print out an error message to stderr, and 2 is returned.

This program opens the files in sequence, reads their contents in chunks and then prints their contents to stdout. 

### my-sed.c

The my-sed.c program uses regular expressions to match and replace terms in a data stream described to the program. After replacing terms, the modified data stream is written to stdout. After compiling, my-sed takes the following arguments from the command line:

`./my-sed find_term replace_term [file ...]`

The find_term is the substring to find in the data stream, and the replace_term is what the find_term will be replaced with. If files are given, my-sed will run through each file and output their processed data to stdout. If no files are given, my-sed will read from stdin. 

In the case that an error occurs, the program will exit with a return code of 1. If the program completed normally, it will have an exit code of 0. If the program closed because not enough arguments were provided, the program prints "my-sed: find term replace term [file ...]" to stdout. If the program closed because a file could not be opened, the message "my-sed: cannot open file" is printed to stdout. 

This program goes through the data in each file or stdin, loops through each character and searches for where replacements should occur. The my-sed program outputs a single character to stdout from the input at a time, and then outputs characters in the replace term when the find_term is discovered.

### my-uniq.c

The my-uniq.c program mimics the behavior of the UNIX utility called uniq, which outputs each unique line from a file to stdout. After compiling, the my-uniq program takes the following arguments from the command line:

`./my-uniq [file ...]`

Where my-uniq can take many files to process in sequence. When my-uniq switches from file to file, lines do not carry over, so a similar ending line in the first file will not be compared to the first line in the next file.

If one of the files could not be opened, the program closes with an exit code of 1 and prints "my-uniq: cannot open file" to stdout. If any other errors occur, the global errno value is used to write the error to stderr and the program closes with an exit code of 2. If the program does not encounter issues, the program will close with a return code of 0. 

The my-uniq program functions by grabbing one line at a time from the input files, comparing two lines in sequence at a time, and printing if the lines differ or if it's the first line. 
