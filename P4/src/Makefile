executable=simulate
objects=doublell.o queue.o
CFLAGS=-Wall -Werror

default: ${executable}

${executable}: main.c ${objects}
	gcc ${CFLAGS} -o ${executable} main.c ${objects}

doublell.o: doublell.c
	gcc ${CFLAGS} -c doublell.c

queue.o: queue.c
	gcc ${CFLAGS} -c queue.c

clean: ${executable}
	rm ${executable} ${objects}
