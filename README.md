# Paging-Simulation

## Team Members

- Christian Galvez
- Ethan Pongon
- Ming Xia

## Compilation and Running Instructions

Running `make` from the project directory will build the executable. Once built, simulations can be run by executing the following:

`./simulate <FIFO | LRU | MIN> <cache size> <input file>`

Where the first argument is the page replacement algorithm to use. Currently this program supports FIFO, LRU and MIN. The second argument is the cache size, which should be a positive number ranging from 0 to LONG_MAX (2<sup>64</sup> - 1). The final argument is the input file containing the list of read/write access to use with the page replacement simulation.

To run the program with the supplied pageref.txt file using LRU with a cache size of 4, the following example command can be executed:

`./simulate LRU 4 pageref.txt`

## What Does Program Input Look Like

Input files for the program are formatted in the following manner:

R1
R2
W3

Where an R signifies read, W signifies write, and the number is the page table entry being referenced. In the above example, we read page table entry 1 and 2, and then write to page table entry 3. This defines the access patterns that we're going to pass into our simulation.

## What The Program Will Output

Upon completion, the program will spit out the following output to stdout:

```
-----------------------------------------
Algorithm:              LRU
References:             550
Page misses:            530
Page miss time units:   2650
Dirty write time units: 1100
Total time:             3750
-----------------------------------------
```

Each line in this output contains information about the simulation that was just run. The information displayed on each line is as follows:

1. Algorithm used. In the above example, the LRU algorithm was used.
2. Total references. In the above example, there were 550 total references.
3. Page table entry misses. In the above example, there were 530 page table misses.
4. Page miss time units. This is effectively page misses * 5, as there's a cost of 5 time units every time a page misses.
5. Dirty write time units. This is the number of dirty pages that are written to disk. The math used to come up with this number is (total # of dirty page evictions) * (10 time units), as every dirty write costs 10 time units. This includes dirty pages left in the queue at the end of the simulation, as they must be written to disk eventually. A dirty page is only written to disk when it is evicted from the queue containing active pages.
6. Total time. This is effectively the dirty write time units + page miss time units.
