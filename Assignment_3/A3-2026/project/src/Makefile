CC=gcc
CFLAGS= -pthread
FMT=indent
RM=rm -f

# Default values
framesize ?= 12
varmemsize ?= 20

DEFS = -D FRAME_STORE_SIZE=$(framesize) -D VAR_MEM_SIZE=$(varmemsize)

mysh: shell.c interpreter.c shellmemory.c pcb.c readyqueue.c scheduler.c
	$(CC) $(CFLAGS) $(DEFS) -c shell.c interpreter.c shellmemory.c pcb.c readyqueue.c scheduler.c
	$(CC) $(CFLAGS) $(DEFS) -o mysh shell.o interpreter.o shellmemory.o pcb.o readyqueue.o scheduler.o

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

