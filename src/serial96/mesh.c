/*************************************************************

   Created:  September 17, 1997
   Written by:  Osama Salem
   Modified:

**************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "kestrel.h"

#define MESH_ERROR 3


extern Instruction instruction;
extern PE_ptr LinearArray;

int iproc;			/* number of PE ...oasma...*/


void EquateNonZero(int count, int sourcePE)
{
  (LinearArray+count)->simWires.flag =
   ((((LinearArray+sourcePE)->simWires.bsLatch >> 7) ^
	instruction.finv) & 1);
}


void EquateNonZeroOld(int count, int sourcePE)
{
  (LinearArray+count)->simWires.oldBs =
	(LinearArray+count)->simWires.bsLatch;

  (LinearArray+count)->simWires.flag =
   ((((LinearArray+sourcePE)->simWires.oldBs >> 7) ^
	instruction.finv) & 1);
}


void EquateZero(int count)
{
  (LinearArray+count)->simWires.flag =
	((0 ^ instruction.finv) & 1);
}


void EquateZeroOld(int count)
{
  (LinearArray+count)->simWires.oldBs =
	(LinearArray+count)->simWires.bsLatch;

  (LinearArray+count)->simWires.flag =
	((0 ^ instruction.finv) & 1);
}


/* osama, D1 Mesh operation */
void D1FromSouth(int count)
{
   if ((count % 64) < 56)
     EquateNonZero(count, (count + 8));

   else
     EquateZero(count);
}


/* osama, D2 Mesh operation */
void D2FromSouth(int count)
{
   int peNumber = count % 64;
   int rowNumber = peNumber / 8;
   int sourcePE;

   if (rowNumber > 5)
   { 
     EquateZero(count);
     return;
   }

   if (rowNumber < 2)
     sourcePE = count + 8 * (2 - rowNumber);

   else
     sourcePE = count + 8 * (6 - rowNumber);

   EquateNonZero(count, sourcePE);
}


/* osama, D4 Mesh operation */
void D4FromSouth(int count)
{
   int peNumber = count % 64;
   int rowNumber = peNumber / 8;
   int sourcePE;

   if (rowNumber > 3) 
     EquateZero(count);

   else
   {
     sourcePE = count + 8 * (4 - rowNumber);
     EquateNonZero(count, sourcePE);
   }
}


/* osama, R1 Mesh operation */
void R1FromEast(int count)
{   
   if ((count % 64) == 63)
     EquateZero(count);
      
   else
     EquateNonZero(count, (count + 1));
}


/* osama, R2 Mesh operation */
void R2FromEast(int count)
{
  int peNumber = count % 64;
  int columnNumber = peNumber % 8;
  int sourcePE;

  if (peNumber > 61)
  {
    EquateZero(count);
    return;
  }
  
  if (columnNumber > 1 && columnNumber < 6)
    sourcePE = count - columnNumber + 6;

  else
  {
    if (columnNumber > 5)
      sourcePE = count - columnNumber + 10;

    else
      sourcePE = count - columnNumber + 2;
  }
  
  EquateNonZero(count, sourcePE);
}


/* osama, R4 Mesh operation */
void R4FromEast(int count)
{   
  int peNumber = count % 64;
  int columnNumber = peNumber % 8;
  int sourcePE;

  if (peNumber > 59)
  {
    EquateZero(count);
    return;
  }
  
  if (columnNumber < 4)
    sourcePE = count - columnNumber + 4;

  else
    sourcePE = count - columnNumber + 12;

  EquateNonZero(count, sourcePE);
}


/* osama, L1 Mesh operation */
void L1FromWest(int count)
{   
  if (count % 64 == 0)
    EquateZeroOld(count);

  else
    EquateNonZeroOld(count, (count - 1));
}


/* osama, L2 Mesh operation */
void L2FromWest(int count)
{
  int peNumber = count % 64;
  int columnNumber = peNumber % 8;
  int sourcePE;

  if (peNumber == 0)
  {
    EquateZeroOld(count);
    return;
  }
  
  if (columnNumber > 4)
    sourcePE = count + 4 - columnNumber;

  else
  {
    if (columnNumber == 0)
	sourcePE = count - 4;
    else
    	sourcePE = count - columnNumber;
  }

  EquateNonZeroOld(count, sourcePE);
}


/* osama, L4 Mesh operation */
void L4FromWest(int count)
{
  int peNumber = count % 64;
  int columnNumber = peNumber % 8;
  int sourcePE;

  if (peNumber == 0)
  {
    EquateZeroOld(count);
    return;
  }

  if (columnNumber == 0)
    sourcePE = count - 8;
  
  else
    sourcePE = count - columnNumber;

  EquateNonZeroOld(count, sourcePE);
}


/* osama, U1 Mesh operation */
void U1FromNorth(int count)
{
  int rowNumber = (count % 64) / 8;
  int sourcePE;

  if (rowNumber == 0)
  { 
    EquateZeroOld(count);
    return;
  }

  sourcePE = count - 8;
  EquateNonZeroOld(count, sourcePE);
}


/* osama, U2 Mesh operation */
void U2FromNorth(int count)
{
  int peNumber = count % 64;
  int rowNumber = peNumber / 8;
  int columnNumber = peNumber % 8;
  int sourcePE;
  
  if (rowNumber == 0)
  {
    EquateZeroOld(count);
    return;
  }
   
  if (rowNumber < 5)
    sourcePE = count - peNumber + columnNumber;

  else
    sourcePE = count - peNumber + 32 + columnNumber;

  EquateNonZeroOld(count, sourcePE);
}


/* osama, U4 Mesh operation */
void U4FromNorth(int count)
{
  int peNumber = count % 64;
  int rowNumber = peNumber / 8;
  int columnNumber = peNumber % 8;
  int sourcePE;
 
  if (rowNumber == 0)
  {
    EquateZeroOld(count);
    return;
  }

  sourcePE = count - peNumber + columnNumber;
  EquateNonZeroOld(count, sourcePE);
}

void ProcessMesh(int count)
{
  if (iproc % 64)
  {
    fprintf(stderr, "\nERROR: Number of PEs must be a multiple\
of 64 in order to process a MESH operations");

    exit (MESH_ERROR);
  }

  switch (instruction.fb)
  {
    case 20 :
	L1FromWest(count);
	break;

    case 21 :
    	R1FromEast(count);
	break;

    case 22 :
    	U1FromNorth(count);
	break;

    case 23 :
    	D1FromSouth(count);
	break;
	
    case 24 :
    	L2FromWest(count);
	break;

    case 25 :
    	R2FromEast(count);
	break;

    case 26 :
    	U2FromNorth(count);
	break;

    case 27 :
    	D2FromSouth(count);
	break;

    case 28 :
    	L4FromWest(count);
    	break;

    case 29 :
    	R4FromEast(count);
    	break;

    case 30 :
    	U4FromNorth(count);
    	break;

    case 31 :
    	D4FromSouth(count);
    	break;
  }
}
