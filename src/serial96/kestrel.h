#include "kesboard.h"

#ifndef _KESTREL_H_
#define _KESTREL_H_

/*****************************************************************************
   Define contstants.
******************************************************************************/
#define REG_FILE_SIZE 32
#define SRAM_SIZE 256
#define PES_PER_CHIP 64
#define CHIPS_PER_BOARD 8

/*****************************************************************************
   Declare structures to be used as microinstruction latches and for
   storing the program in an array.
   1/29/98  we are no longer going to use this version of the instruction.
      --Jennifer Leech
      1/31/98 --microInst structure definition deleted.
****************************************************************************/



typedef struct
{
  int numChips;		/* osama */
  int totalPE;		/* osama */
  /*  int microCounter;*/
  int instCount;
  char log;
  char test;
  char output;
} data;

typedef struct
{
  unsigned char address;
  unsigned char alu;
  unsigned char ats;
  unsigned char cCOut;
  unsigned char cIn;
  unsigned char aCOut;
  unsigned char cts;
  unsigned char flag;
  unsigned char msb;
  unsigned char multLo;
  unsigned char operandA;	
  unsigned char operandB;
  unsigned char operandC;
  unsigned char result;
  unsigned char mask;
  unsigned char bsLatch;
  unsigned char cLatch;
  unsigned char maskLatch;
  unsigned char mdrLatch;
  unsigned char multHiLatch;
  unsigned char minLatch;
  unsigned char eq;
  unsigned char eqLatch;
  unsigned char nor;
  unsigned char oldBs;
} wires;

typedef struct
{
  char *logName;
  char *stageInName;
  char *stageOutName;
  char *programName;
  FILE *stageInFile;
  FILE *stageOutFile;
  FILE *testFile;
  FILE *programFile;
  FILE *logFile;
} files;

/* NEW and EXCITING data structure for a PE, each instance of a PE,
    number of instances of the PE data structure is allocated dynamically,
   has its own memory space allocated for sram,ssr's, and simulated wires */

/* PE Linear Array Structure */
typedef struct PE {
 
  unsigned char rightRegFile[REG_FILE_SIZE];
  unsigned char rightRegFileFlag[REG_FILE_SIZE]; /* Has this register been used ??? */
  unsigned char sram[SRAM_SIZE];
  wires simWires;

} PE;


typedef struct PE* PE_ptr; 

extern int totalInst;
extern data simData;
extern files simFiles;
extern PE_ptr LinearArray;
extern unsigned char leftmostRegFile[REG_FILE_SIZE];
extern unsigned char leftmostRegFileFlag[REG_FILE_SIZE];

void TriggerIRQ(void);

#endif
