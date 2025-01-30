objects=doublell.o fcfs.o pr.o rr.o sjf.o
executable=prog
CFLAGS=-Wall -Werror
LFLAGS=-pthread

default: ${objects} ${executable}

prog: main.c ${objects}
	gcc -g -o prog main.c ${objects} ${CFLAGS} ${LFLAGS}

doublell.o: doublell.c
	gcc -c doublell.c ${CFLAGS}

fcfs.o: fcfs.c
	gcc -c fcfs.c ${CFLAGS} ${LFLAGS}

pr.o: pr.c
	gcc -c pr.c ${CFLAGS} ${LFLAGS}

rr.o: rr.c
	gcc -c rr.c ${CFLAGS} ${LFLAGS}

sjf.o: sjf.c
	gcc -c sjf.c ${CFLAGS} ${LFLAGS}

clean: ${objects} ${executable}
	rm ${objects} ${executable}
