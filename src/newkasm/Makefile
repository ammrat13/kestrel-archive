
CC=gcc
CFLAGS=-g -Wall -DDATE="\"`date +%D_%T`\"" 
.SUFFIXES: tex
LATEX=latex
L2H = latex2html -info 0 -split 1
EXE=newkasm genmanual

HFILES=opc.def fields.def newkasm.h codetable.h
OFILES= newkasm.o codetable.o genmanual.o


newkasm: newkasm.o codetable.o
	${CC} ${CFLAGS} -o $@ $@.o codetable.o

genmanual:  genmanual.o codetable.o
	${CC} ${CFLAGS} -o $@ $@.o  codetable.o

all:	newkasm kasm_man.ps

install: all kasm_man.ps
	cp newkasm /projects/kestrel/bin/`uname -m`
	cp kasm_man.ps /projects/kestrel/doc

${OFILES}: ${HFILES}

kasmopc.tex: genmanual 
	genmanual

kasm_man.dvi:  kasmopc.tex kasm_man.tex
	touch kasm_man.ind
	${LATEX} kasm_man.tex </dev/null
	makeindex kasm_man.idx
	${LATEX} kasm_man.tex </dev/null
	${LATEX} kasm_man.tex </dev/null

kasm_man.html: kasm_man.dvi
	${L2H} kasm_man

xdvi: kasm_man.dvi
	xdvi kasm_man.dvi

kasm_man.ps:   kasm_man.dvi
	dvips kasm_man -o


test:  test.kasm newkasm
	newkasm -g -b test



clean:
	/bin/rm -f *.o *~ *.dvi *.ps *.idx *.log *.ilg *.ind *.aux $(EXE) core

