# CPU-Scheduling

## Group Members:
* Christian Galvez
* Ethan Pongon
* Ming Xia

## Files

### doublell.c

doublell.c is a file containing functions to operate on a doubly linked list.
This linked list structure is used for the ready queue and wait queue.

### doublell.h

doublell.h is a header file for doublell.c that contains the doubeLL struct
definition and prototypes for the doubly linked list functions.

### fcfs.c

fcfs.c contains an fcfs function that takes a pointer to a settings struct
(defined in schedulers.h) and schedules CPU jobs sitting in the ready queue
using the FCFS algorithm.

### input.txt

input.txt contains input to be used with the scheduling algorithm functions.

### main.c

main.c contains the code that starts up the three threads (IO, file read,
scheduling), initializes semaphores and pthreads. After each thread has exited,
the main function will join the threads and print out performance statistics.

### Makefile

Makefile compiles all of the object files (fcfs.o, pr.o, rr.o, sjf.o,
doublell.o) and the prog executable after issuing the `make` command. The object
files and the compiled executable can be cleaned by issuing `make clean`.

### pr.c

pr.c contains a pr function that takes a pointer to a settings struct
(defined in schedulers.h) and schedules CPU jobs sitting in the ready queue
using the PR algorithm.

### README.md

This README file.

### rr.c

rr.c contains a rr function that takes a pointer to a settings struct
(defined in schedulers.h) and schedules CPU jobs sitting in the ready queue
using the RR algorithm.

### schedulers.h

Header file containing prototype functions for FCFS, RR, PR, and SJF functions.
This header file also contains external references for the semaphores used in
the main.c file to protect shared resources.

### sjf.c

sjf.c contains a sjf function that takes a pointer to a settings struct
(defined in schedulers.h) and schedules CPU jobs sitting in the ready queue
using the SJF algorithm.

## Implementation

The main.c file contains the main() function that acts as the entry point for
our our executable. Inside of this main function arguments are parsed,
semaphores for protection shared resources are initialized, pthreads are created
and performance metrics are printed (after all threads join).

In the main function, a settings struct (as defined in schedulers.h) is created
and zeroed out before being configured. This struct holds information about the
current algorithm being ran, the quantum, input file and performance metric
data. This struct also contained pointers for the ready queue and waiting queue,
which were pointers to doubleLL structs. Zeroing out the settings struct
instance in main was necessary because the doubleLL functions in doublell.c
require that the doubly linked list pointer starts out as NULL (0 on Linux).

This settings struct, named info in main, was passed around to the CPU
scheduling thread, the file read thread and the IO thread. Accesses to this
struct that could possibly occur at the same time across multiple threads were
protected with an access semaphore. This included writing to variables in the
info struct or performing doubleLL functions.

The file read thread utilized the fileReadRoutine function in the main.c file,
which worked by opening the given input file, parsing its contents with
getline() and strok() functions. Information retrieved from the input file was
used to create PCBs, and those PCBs were inserted into the ready queue. PCB, or
process control block data, is stored inside of a PCB struct as defined in the
schedulers.h file.

The io thread utilized the ioRoutine function to grab values from the waiting
queue and process them in FIFO order. For each item process, the thread slept
for the given time in ms and then moved the next process back onto the ready
queue. After an item is inserted into the ready queue, the IO thread signals
to the CPU scheduling thread that a job has become available. This was done by
posting a value to increment the queue semaphores. The queue semaphores are
an array of semaphore handle values declared at the top of the main.c file. The
first queue semaphore value at index 0 is the ready queue semaphore. The second
queue semaphore value at index 1 is the waiting queue semaphore. When a
either queue semaphores become available, their respective processes are
signaled that an item is available.

The CPU scheduling thread utilized the cpuRoutine function that contains a list
of function pointers corresponding to the current algorithm to use for
scheduling processes. This thread runs the respective scheduling algorithm and
exits when the exit condition in the scheduling algorithm was met and 1 is
returned. When a process is being processed, the thread sleeps for the given
amount of time.

fcfs(), rr(), pr() and sjf() run their own respective CPU scheduling algorithm.
Each function takes a reference to the (settings *) info struct defined in main
and operates on the ready/waiting queues provided. When each of these functions
continue to run, they return 0. When scheduling as completed, they each return 1
and the cpuRoutine infinite loop ends.

In main, after pthread_join is used to join all three threads, performance
metrics are pulled from information in the (settings *) info struct. Performance
metrics in the (settings *) info struct are pulled from each individual thread,
using the clock_gettime() function at various points to measure time for a
simulated process. These performance metrics are printed to stdout and the
program is closed.

## Compiling and Running

To compile the program, please issue `make` to compile the `prog` executable.
After compilation, run the `prog` program with the following command line
arguments:

`-alg [FIFO/SJF/RR/PR]` to select algorithm to use.
`-quantum n`, required by round robin (RR), where n is the quantum value.
`-input input-file` where input-file is the name of the input file to use.

To clean object files and executable, run `make clean`. 
