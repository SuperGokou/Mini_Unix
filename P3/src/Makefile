executable=BackItUp
objects=BackItUp.o
sources=main.c
CFLAGS=-Wall -Werror
LFLAGS=-pthread

default: ${executable} ${objects}

${executable}: ${sources} ${objects}
	gcc -o ${executable} ${sources} ${objects} ${CFLAGS} ${LFLAGS}

BackItUp.o: BackItUp.c
	gcc -c BackItUp.c ${CFLAGS} ${LFLAGS}

clean: ${executable} ${objects}
	rm ${executable} ${objects}
