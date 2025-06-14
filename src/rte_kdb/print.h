/* $Id: print.h,v 1.4 1998/05/30 17:39:02 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * print.h
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "format.h"
#include "precision.h"

#ifndef _PRINT_H_
#define _PRINT_H_

/* used by printMultibit to select a mutliprecision number
 * from the sram or register bank */
#define PRINT_REG   1
#define PRINT_SRAM  2

/* arguments to printRange to indicate what value from
 * each processing element to display to the user */
#define PRINT_TYPE_BSLATCH 1
#define PRINT_TYPE_CLATCH  2
#define PRINT_TYPE_EQLATCH 3
#define PRINT_TYPE_MASKLATCH 4
#define PRINT_TYPE_MDRLATCH 5
#define PRINT_TYPE_MINLATCH 6
#define PRINT_TYPE_MULTHILATCH 7
#define PRINT_TYPE_REGISTER 8
#define PRINT_TYPE_SRAM 9
#define PRINT_TYPE_ALL 10 /* used by FetchStateFromArray */

void printRangeMenu(void);
void whatRange(UserProgram *program);
void printRegisterRange(UserProgram *program);
void printSramRange(UserProgram *program);
void examinePE(UserProgram *program);
void printMultibit(UserProgram *program, int print_type,
		   int * data, int starting_reg);
int printMPnumber(int * data, int precision);
void printElement(UserProgram *program, int pe_number);
void printRange(UserProgram *program, int print_type, int location);
void PrintMPElement(UserProgram *program, int PEindex);
void printFormatInformation(UserProgram *program);
void printFormat(char format);
void printEightbit(int value, char format);
void printHex(int value, int precision);
void printBinary(int value, int precision);
void printDNA(int value);
void printRNA(int value);
void printProtein(int value);
void printUsage(void);
void help(void);
void printMenu(void);

#endif



