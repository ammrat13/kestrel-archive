#	Justin Meyer	justin@cse.ucsc.edu
#	Jennifer Leech  jleech@cse.ucsc.edu
#
#	Kestrel Serial Simulator Makefile 
#	written 10/15/97

UMASK=007; export UMASK
###################### Compiler Macros ######################

CC =		gcc
CCOPTS =	-O2  -ansi -Wall -pedantic -g -D__USE_FIXED_PROTOTYPES__
OBJLIBS =

#################### Other Macros ##########################

UMASK =		umask 007

##################### Source File Macros ####################


HEADERS =	kestrel.h kesboard.h mesh.h controller.h
C_SRC =		execute.c kestrel.c kesboard.c mesh.c controller.c
C_OBJS =	execute.o kestrel.o kesboard.o mesh.o controller.o
C_DEPS =	execute.d kestrel.d kesboard.d mesh.d controller.d
C_TARGETS =	serial

SRC =		$(C_SRC)
OBJECTS =	$(C_OBJS)
DEPS =		$(C_DEPS)
TARGETS =	$(C_TARGETS)


#################### The Actual Makefile ###################

all: $(TARGETS) $(HEADERS) 

clean:
	-rm -f $(OBJECTS) $(DEPS)
	-rm -i *~
	-rm -i core

spotless: clean
	-rm -f $(TARGETS)

checkout:
	-co $(SRC) $(HEADERS)

nuke: clean spotless


serial: $(OBJECTS)
	@$(UMASK)
	$(CC) $(OBJECTS) $(OBJLIBS) -o serial -lm

kestrel.o: kestrel.c $(HEADERS)
	@$(UMASK)
	$(CC) $(CCOPTS) kestrel.c -c

execute.o: execute.c $(HEADERS)
	@$(UMASK)
	$(CC) $(CCOPTS) execute.c -c

mesh.o: mesh.c mesh.h
	@$(UMASK)
	$(CC) $(CCOPTS) mesh.c -c

kesboard.o: kesboard.c kesboard.h
	@$(UMASK)
	$(CC) $(CCOPTS) kesboard.c -c

controller.o: controller.c controller.h kesboard.h
	@$(UMASK)
	$(CC) $(CCOPTS) controller.c -c
