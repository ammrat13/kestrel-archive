/* $Id: execute.c,v 1.25 1999/04/22 18:52:53 dmdahle Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "kestrel.h"
#include "mesh.h"
#include "kesboard.h"
#include "controller.h"


void TestFile(void);


#define DEBUG 0


void WOR(int count)
{
  int wor_pe, chip_number;

  wor_pe = ((LinearArray[count].simWires.maskLatch
	     && (LinearArray[count].simWires.bsLatch >>7)) & 1);

  chip_number = (count / PES_PER_CHIP) % CHIPS_PER_BOARD;

  controller.wor |= (wor_pe << chip_number);
}


/* This must be called before any computation takes place.  This
controls conditional execution.  If a PE has a mask of 1, it will execute the
instruction, meaning registers, SRAM and latches will be updated as necessary.
If a PEs mask is 0, it will still compute a result, but the result will not
be stored, and no latches or SRAM location will be modified.
*/
void Mask(int count)
{     
   LinearArray[count].simWires.mask = 
     (LinearArray[count].simWires.maskLatch | instruction.force) & 
                                        ~instruction.nop & 1;
}


void MaskFlagGenerator(int count)
{
/* This determines the value put in maskLatch, which controls conditional
computation.  The bit field of the instruction controls what is done with
maskLatch.

  bit		maskLatch			BitShifter

  0000		unchanged			unchanged
  0001          complement of old maskLatch	unchanged (notm)
  0010          flag				unchanged (flagm)
  0011		unchanged			condls	(mask==1 only)
  0100		nor				ors
  0101		nor				ands
  0110		nor				nots
  0111		nor				sets
  1000		unchanged			clear
  1001		nor				clearm
  1010		nor				popnots
  1011          nor				pop
  1100		nor				uncondl
  1101		unchanged			condl	(mask==1 only)
  1110		nor				push
  1111		unchanged			condrs	(mask==1 only)

*/

  if (instruction.nop == 0) {
     if (instruction.bit == 1) {
       LinearArray[count].simWires.maskLatch = 
	 (~(LinearArray[count].simWires.maskLatch) & 1);
     } else if (instruction.bit == 2) {
       LinearArray[count].simWires.maskLatch = LinearArray[count].simWires.flag;
     } else {
         if ((instruction.bit != 0) && (instruction.bit != 3) &&
	     (instruction.bit != 8) && (instruction.bit != 13) &&
	     (instruction.bit != 15)) {
	   LinearArray[count].simWires.maskLatch = LinearArray[count].simWires.nor;
	 }
     }
  }
}

void OperandA(int count)
{
/* Choose operandA based on opA field.  An msb of 0 means the value comes
from the left register file.  An msb of 1 means the value comes from the right
register file.
*/


  if(instruction.opA < 32)
    {
      if (count == 0 )
       LinearArray[0].simWires.operandA = leftmostRegFile[(instruction.opA & 31)];           
      else
       LinearArray[count].simWires.operandA = 
	 LinearArray[count-1].rightRegFile[(instruction.opA & 31)];
    }
      
  else
    {
      LinearArray[count].simWires.operandA = 
	LinearArray[count].rightRegFile[(instruction.opA & 31)];  
    }

}


void OperandB(int count)
{
/* Choose operandB based on opB field.

  opB			operandB

  000			operandC    
  001                   msb of operandC
  010                   mdrLatch
  011                   msb of mdrLatch
  100                   multHiLatch
  101			msb of multHiLatch
  110                   bsLatch
  111			imm
*/

  if(instruction.opB == 0)
    {
      LinearArray[count].simWires.operandB = LinearArray[count].simWires.operandC;
    }

  else if(instruction.opB == 1)
    {
      if(LinearArray[count].simWires.operandC > 127)
	{
	  LinearArray[count].simWires.operandB = 255;
	}

      else
	{
	  LinearArray[count].simWires.operandB = 0;
	}
    }

  else if(instruction.opB == 2)
    {
      LinearArray[count].simWires.operandB = LinearArray[count].simWires.mdrLatch;
    }

  else if(instruction.opB == 3)
    {
      if( LinearArray[count].simWires.mdrLatch > 127)
	{
	  LinearArray[count].simWires.operandB = 255;
	}

      else
	{
	  LinearArray[count].simWires.operandB = 0;
	}
    }

  else if(instruction.opB == 4)
    {
      LinearArray[count].simWires.operandB = LinearArray[count].simWires.multHiLatch;
    }

  else if(instruction.opB == 5)
    {
      if( LinearArray[count].simWires.multHiLatch > 127)
	{
	  LinearArray[count].simWires.operandB = 255;
	}

      else
	{
	  LinearArray[count].simWires.operandB = 0;
	}
    }

  else if(instruction.opB == 6)
    {
      LinearArray[count].simWires.operandB = LinearArray[count].simWires.bsLatch;
    }

  else
    {
      LinearArray[count].simWires.operandB = controller.ArrayImmediate;
    }
}


void OperandC(int count)
{
/* Choose operandC based on opC field.  An msb of 0 means the value comes
from the left register file.  An msb of 1 means the value comes from the right
register file.
*/

  if(instruction.opC < 32) 
    {
      if (count == 0) {
          LinearArray[0].simWires.operandC = leftmostRegFile[(instruction.opC & 31)];
      } else {
      	  LinearArray[count].simWires.operandC = 
	    LinearArray[count-1].rightRegFile[(instruction.opC & 31)];
      }
    }
      
  else
    {
      LinearArray[count].simWires.operandC = 
	LinearArray[count].rightRegFile[(instruction.opC & 31)];  
    }
}


void AddressGenerator(int count)
{
/* Set up address to access SRAM.

  rm			address

  00			imm + operandC
  01                    imm
  10                    imm
  11                    imm
*/

  if(instruction.rm == 0)
    {
      LinearArray[count].simWires.address = 
	controller.ArrayImmediate + (LinearArray[count].simWires.operandC);
    }

  else
    {
      LinearArray[count].simWires.address = controller.ArrayImmediate;
    }
}


void ALU(int count)
{
/* Compute alu based on func field.  If mp is 1, then the initial carry in,
cIn comes from cLatch.  Otherwise, it comes from the ci field of the
instruction.  If lc is 1 and mask is 1, then the final aCOut is stored in
cLatch. 
*/
  int i;                /* osama */
  char f0,f1,f2,f3,f4;

  char aBit;
  char bBit;   
  char g;
  char k;
  char p;
  char c;

  if(instruction.mp)
    {
      LinearArray[count].simWires.cIn = LinearArray[count].simWires.cLatch;
    }
 
  else
    {
      LinearArray[count].simWires.cIn = instruction.ci;
    }
 
  c = LinearArray[count].simWires.cIn;
  LinearArray[count].simWires.alu = 0;

  f0 = (instruction.func & 1);
  f1 = ((instruction.func & 2) >> 1);
  f2 = ((instruction.func & 4) >> 2);
  f3 = ((instruction.func & 8) >> 3);
  f4 = ((instruction.func & 16) >> 4);

  for(i = 0;i < 8;i++)
    {
      aBit = ((LinearArray[count].simWires.operandA & (1 <<i)) >> i);
      bBit = ((LinearArray[count].simWires.operandB & (1 <<i)) >> i);
 
      g = (((((f0 && aBit) || (f1 && !aBit)) && bBit) ||
            (aBit && f2 && !bBit)) && f4);
 
      k = (!(f3 && f4) && ((((f3 && aBit) ||
                             (f2 && !aBit)) && bBit) ||
                           (((f1 && aBit) ||
                             (f0 && !aBit)) && !bBit)));
 
      p = ((~(g | k)) & 1);
      LinearArray[count].simWires.alu = (LinearArray[count].simWires.alu | ((p ^ c) << i));
      c = (((~k & 1) & c) | g);
    }
  
  LinearArray[count].simWires.aCOut = c;
  LinearArray[count].simWires.ats = (LinearArray[count].simWires.aCOut ^ p);

}


/*osama*/
void FlagSelector(int count)
{
  int my_wor;
/* The fb instruction field indicates which signal to place
   on the flag bus.  Comparator has special encodings for
   multi-precision comparison (non-reset case).

  fb		flag
  ------------------
  Comparator MUX
  00000		cCOut   reset
  00001		cts	reset
  00010		msb	reset
  00011		minLatch
  00100		cCOut   non-reset  (multiprecision)
  00101		cts	non-reset
  00110		msb	non-reset
  00111		minLatch (reset, no eqLatch dependence)

  Non-reset values give value if eqLatch=1, and minLatch if eqLatch=0.
  NOTE: comparator will modify eqLatch and minLatch 
          if fb < 3 || (fb > 3 && fb < 7 && eqlatch == 1)

  BitShifter MUX
  01000		nor
  01001		bs0
  01010		bs7
  01011		wor

  ALU MUX
  01100		ats
  01101		aCOut
  01110		cLatch
  01111		0

  NOTE: cLatch modified when lc == 1.

  Equal MUX
  10000		eq
  10001		eqLatch
  10010		Unused
  10011		Unused

  Mesh MUX
  10100		+1 (get from west)
  10101		-1 (get from east)
  10110		+8 (get from north)
  10111		-8 (get from south)
  11000		+2 (get from west)
  11001		-2 (get from east)
  11010		+16 (get from north)
  11011		-16 (get from south)
  11100		+4 (get from west)	
  11101		-4 (get from east)
  11110		+32 (get from north)
  11111		-32 (get from south)
*/

  if ((instruction.fb == 0) || 
      ((instruction.fb == 4) && (LinearArray[count].simWires.eqLatch))) {
    LinearArray[count].simWires.flag = 
	((LinearArray[count].simWires.cCOut ^ instruction.finv) & 1);
  }
  if ((instruction.fb == 1) || 
      ((instruction.fb == 5) && (LinearArray[count].simWires.eqLatch))) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.cts ^ instruction.finv) & 1);
  }
  if ((instruction.fb == 2) || 
      ((instruction.fb == 6) && (LinearArray[count].simWires.eqLatch))) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.msb ^ instruction.finv) & 1);
  }
  if (((!LinearArray[count].simWires.eqLatch) && (instruction.fb > 3) && 
       (instruction.fb < 7)) || (instruction.fb == 3) || (instruction.fb == 7)) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.minLatch ^ instruction.finv) & 1);
  }
  if (instruction.fb == 8) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.nor ^ instruction.finv) & 1);
  }
  if (instruction.fb == 9) {
    LinearArray[count].simWires.flag = 
      (((LinearArray[count].simWires.bsLatch & 1) ^ instruction.finv) & 1);
  }
  if (instruction.fb == 10) {
    LinearArray[count].simWires.flag = 
      ((((LinearArray[count].simWires.bsLatch >> 7) & 1) ^ instruction.finv) & 1);
  }
  if (instruction.fb == 11) {
    /* Use the wired-or from this chip only. */
    my_wor = (controller.wor >> ((count / PES_PER_CHIP) % CHIPS_PER_BOARD)) & 1;
    LinearArray[count].simWires.flag = ((my_wor ^ instruction.finv) & 1); 
  }
  if (instruction.fb == 12) {
      LinearArray[count].simWires.flag = 
	((LinearArray[count].simWires.ats ^ instruction.finv) & 1);
  }
  if (instruction.fb == 13) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.aCOut ^ instruction.finv) & 1);
  }
  if (instruction.fb == 14) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.cLatch ^ instruction.finv) & 1);
  }
  if (instruction.fb == 15) {
    LinearArray[count].simWires.flag = ((0 ^ instruction.finv) & 1);
  }
  if (instruction.fb == 16) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.eq ^ instruction.finv) & 1);
  }
  if (instruction.fb == 17) {
    LinearArray[count].simWires.flag = 
      ((LinearArray[count].simWires.eqLatch ^ instruction.finv) & 1);
  }

  if (instruction.fb >= 20 && instruction.fb <= 31) {
    ProcessMesh(count);	/* osama, Mesh Operations */
  }

}


void Comparator(int count)
{
  unsigned int compare;
  unsigned char operandCInv;

  operandCInv = ~(LinearArray[count].simWires.operandC);

  compare = LinearArray[count].simWires.alu + operandCInv + 1;

  LinearArray[count].simWires.cCOut = (((compare) >> 8) & 1);
  LinearArray[count].simWires.msb = ((compare >> 7) & 1);
  LinearArray[count].simWires.cts = 
    (((compare >> 8) & 1) ^ ((( LinearArray[count].simWires.alu >> 7) & 1) 
			     ^ ((operandCInv >> 7) & 1)));

  if (LinearArray[count].simWires.alu == LinearArray[count].simWires.operandC) {
    LinearArray[count].simWires.eq = 1;
  } else {
    LinearArray[count].simWires.eq = 0;
  }
}


void ResultSelector(int count)
{

  if((instruction.mp == 1) && (instruction.ci == 1))
    {
      LinearArray[count].simWires.result = LinearArray[count].simWires.multLo;
    }
  
  else if(instruction.rm == 3)
    {
      if( LinearArray[count].simWires.flag == 1)	/*osama */
	{
	  LinearArray[count].simWires.result = LinearArray[count].simWires.alu;
	  /* printf("sim: alu 487 result is [%d]\n", LinearArray[count].simWires.result); */
	}

      else
	{
	  LinearArray[count].simWires.result = LinearArray[count].simWires.operandC;
	 /*  printf("sim: operandC493 result is [%d]\n",LinearArray[count].simWires.result); */
	}
    }

  else if(instruction.rm == 2)
    {
      LinearArray[count].simWires.result = LinearArray[count].simWires.operandC;
     /*  printf("sim: operandC500 result is [%d]\n",LinearArray[count].simWires.result); */
    }

  else
    {
      LinearArray[count].simWires.result = LinearArray[count].simWires.alu;
     /*  printf("sim: ALU506 result is [%d]\n",LinearArray[count].simWires.result); */
    }
}


void Multiplier(int count)
{

  char sign;
  int mult;

     if((instruction.mp == 1) && (instruction.ci == 1) &&
		(LinearArray[count].simWires.mask == 1))
       {
         sign = ((instruction.func >> 1) & 3);

         if(sign == 0)
   	   {
	      mult = LinearArray[count].simWires.operandA * LinearArray[count].simWires.operandB;
	   }

        else if(sign == 1)
      	   {
	     mult = (char)LinearArray[count].simWires.operandA * LinearArray[count].simWires.operandB;
	   }

        else if(sign == 2)
      	   {
	      mult = LinearArray[count].simWires.operandA * (char)LinearArray[count].simWires.operandB;
	   }

        else
	   {
	     mult = (char)LinearArray[count].simWires.operandA * (char)LinearArray[count].simWires.operandB;
	   }

        if(instruction.func & 1)
       	   {
	     mult = (mult + LinearArray[count].simWires.multHiLatch);
	   }
	
       if(instruction.func & 16)
	  {
	    mult = (mult + LinearArray[count].simWires.operandC);
	  }
      
         LinearArray[count].simWires.multLo = (mult & 255);
      
         if(instruction.func & 8)	/* osama */
	   {
	     LinearArray[count].simWires.multHiLatch = ((mult >> 8) & 255);
	   }
       }
}


void BitShifter(int count)
{
if (instruction.nop == 0)
  {
  if(instruction.bit == 4)
    {
      LinearArray[count].simWires.bsLatch = 
          ( LinearArray[count].simWires.bsLatch | ( LinearArray[count].simWires.flag << 7));
    }

  else if(instruction.bit == 5)
    {
      LinearArray[count].simWires.bsLatch = 
         ( LinearArray[count].simWires.bsLatch & (( LinearArray[count].simWires.flag << 7) + 127));
    }

  else if(instruction.bit == 6)
    {
      LinearArray[count].simWires.bsLatch = ( LinearArray[count].simWires.bsLatch ^ 128);
    }

  else if(instruction.bit == 7)
    {
      LinearArray[count].simWires.bsLatch = 
         (( LinearArray[count].simWires.bsLatch & 127) + ( LinearArray[count].simWires.flag << 7));
    }

  else if((instruction.bit == 8) || (instruction.bit == 9))
    {
      LinearArray[count].simWires.bsLatch = 0;
    }

  else if(instruction.bit == 10)
    {
      LinearArray[count].simWires.bsLatch = (( LinearArray[count].simWires.bsLatch ^ 64) << 1);
    }

  else if(instruction.bit == 11)
    {
      LinearArray[count].simWires.bsLatch = ( LinearArray[count].simWires.bsLatch << 1);
    }

  else if(instruction.bit == 12)
    {
      LinearArray[count].simWires.bsLatch = LinearArray[count].simWires.result;
    }

  else if(instruction.bit == 14)
    {
      LinearArray[count].simWires.bsLatch = (( LinearArray[count].simWires.bsLatch >> 1) + 
                                              (LinearArray[count].simWires.flag << 7));
    }

  else 
    {
      if( LinearArray[count].simWires.mask)
	{
	  if(instruction.bit == 3)
	    {
	      LinearArray[count].simWires.bsLatch = 
			(( LinearArray[count].simWires.bsLatch << 1) + LinearArray[count].simWires.flag);
	    }
      
	  else if(instruction.bit == 13)
	    {
	      LinearArray[count].simWires.bsLatch = LinearArray[count].simWires.result;
	    }

	  else if(instruction.bit == 15)
	    {
	      LinearArray[count].simWires.bsLatch = 
				(( LinearArray[count].simWires.bsLatch >> 1) + ( LinearArray[count].simWires.flag << 7));
	    }
	}
    }
  }

  if(! LinearArray[count].simWires.bsLatch)
    {
      LinearArray[count].simWires.nor = 1;
    }

  else
    {
      LinearArray[count].simWires.nor = 0;
    }
}

void UpdateLatchValues(int count)
{
  if ((LinearArray[count].simWires.mask) && (instruction.lc)) {
    LinearArray[count].simWires.cLatch = LinearArray[count].simWires.aCOut;
  }

  /* fb < 4=> always latch comparator flags (reset)
   * fb >= 4 && fb < 8 => latch comparator flags if eqLatch has 1 (non-reset)
   *  this is used for top-down multi-byte comparison.  The values are latched 
   *  until a byte pair is found that is not equal and determines which value
   *  is larger.  No more comparisons are needed after that point, so latching
   *  is disabled for the rest of the comparison */
  if(LinearArray[count].simWires.mask) {
    if((instruction.fb == 0) || ((instruction.fb == 4) && 
				 (LinearArray[count].simWires.eqLatch))) {
      LinearArray[count].simWires.minLatch = LinearArray[count].simWires.cCOut;
    }
    if((instruction.fb == 1) || ((instruction.fb == 5) && 
				 (LinearArray[count].simWires.eqLatch))) {
      LinearArray[count].simWires.minLatch = LinearArray[count].simWires.cts;
    }
    if((instruction.fb == 2) || ((instruction.fb == 6) && 
				 (LinearArray[count].simWires.eqLatch))) {
      LinearArray[count].simWires.minLatch = LinearArray[count].simWires.msb;
    }
    /* can't assign minLatch to minLatch */
  }
  if(( LinearArray[count].simWires.mask) && 
     ((instruction.fb < 3) || (( LinearArray[count].simWires.eqLatch) &&
				(instruction.fb > 3) && (instruction.fb < 7))))  {
    LinearArray[count].simWires.eqLatch = LinearArray[count].simWires.eq;
  }

  /* we can do maskLatch calculation earlier because maskLatch isn't used
   * by anything (it can't be a flag, for example) 
   */
}

void LeftRegFile(void)
{
  if (instruction.dest < 32) {
    if (LinearArray->simWires.mask == 1) {
      leftmostRegFile[(instruction.dest & 31)] = LinearArray->simWires.result;
      leftmostRegFileFlag[(instruction.dest & 31)] = 0;
    } 
    controller.ArrayDataInRight = LinearArray->simWires.result;
    controller.ArrayDinMask = LinearArray->simWires.mask;
  } else if (instruction.dest > 31) {
    if (controller.ArrayDoutMask == 1 && instruction.data_read) {
      leftmostRegFile[(instruction.dest & 31)] = controller.ArrayDataOut;
      leftmostRegFileFlag[(instruction.dest & 31)] = 0;
    }
  }
}


void InnerRegFile(void)
{
  int count;

  for (count = 0 ; count < simData.totalPE ; count++)
  {
     if(LinearArray[count].simWires.mask == 1)
     {
	if (((instruction.dest >> 5) & 1) == 0 && (count != 0))
	{
	    LinearArray[count-1].rightRegFile[(instruction.dest & 31)] =
		LinearArray[count].simWires.result;  
            LinearArray[count-1].rightRegFileFlag[(instruction.dest & 31)] = 0;
	}
    	else if (((instruction.dest >> 5) & 1) == 1)
    	{
	    LinearArray[count].rightRegFile[(instruction.dest & 31)] =
		LinearArray[count].simWires.result;  
            LinearArray[count].rightRegFileFlag[(instruction.dest & 31)] = 0;
        }
     }
  }
}


void RightRegComm(void)
{
  if (instruction.dest < 32) {
    if (controller.ArrayDoutMask == 1 && instruction.data_read) {
      (LinearArray+simData.totalPE-1)->
	rightRegFile[(instruction.dest & 31)] = controller.ArrayDataOut;
      (LinearArray+simData.totalPE-1)->
	rightRegFileFlag[(instruction.dest & 31)] = 0;
    }
  } else if (instruction.dest > 31) {
    controller.ArrayDataInLeft = (LinearArray+(simData.totalPE - 1))->simWires.result;
    controller.ArrayDinMask = (LinearArray+(simData.totalPE - 1))->simWires.mask;
  }
}


void SRAM(int count)
{

  if( LinearArray[count].simWires.mask == 1)
    {
      if(instruction.rd)
	{
	  if(instruction.wr)
	    {
	      LinearArray[count].simWires.mdrLatch = LinearArray[count].simWires.result;
	    }

	  else
	    {
	      LinearArray[count].simWires.mdrLatch = LinearArray[count].sram[LinearArray[count].simWires.address]; 
	    }
	}

      if(instruction.wr)
	{
	  LinearArray[count].sram[LinearArray[count].simWires.address] = LinearArray[count].simWires.result;
	}
    }
}


void LogFile(void)
{
  int pe;
  int j;

  fprintf(simFiles.logFile, "Instruction: %d\n", controller.pc);
  fprintf(simFiles.logFile,"Counter=%d\n", controller.pc);
  fprintf(simFiles.logFile,"Instr: %s\n", progmem[controller.pc]);
  fprintf(simFiles.logFile,"imm=%d, ", instruction.imm);
  fprintf(simFiles.logFile,"dest=%d, ", instruction.dest);
  fprintf(simFiles.logFile,"opC=%d, ", instruction.opC);
  fprintf(simFiles.logFile,"opA=%d, ", instruction.opA);
  fprintf(simFiles.logFile,"opB=%d, ", instruction.opB);
  fprintf(simFiles.logFile,"wr=%d, ", instruction.wr);
  fprintf(simFiles.logFile,"rd=%d, ", instruction.rd);
  fprintf(simFiles.logFile,"rm=%d, ", instruction.rm);
  fprintf(simFiles.logFile,"bit=%d", instruction.bit);
  fprintf(simFiles.logFile,"\nfb=%d, ", instruction.fb);
  fprintf(simFiles.logFile,"finv=%d, ", instruction.finv);
  fprintf(simFiles.logFile,"lc=%d, ", instruction.lc);
  fprintf(simFiles.logFile,"mp=%d, ", instruction.mp);
  fprintf(simFiles.logFile,"ci=%d, ", instruction.ci);
  fprintf(simFiles.logFile,"force=%d, ", instruction.force);
  fprintf(simFiles.logFile, "dinleft: %d dinright: %d dout: %d imm: %d domask: %d dinmask: %d\n",
	  controller.ArrayDataInLeft, controller.ArrayDataInRight,
	  controller.ArrayDataOut, controller.ArrayImmediate,
	  controller.ArrayDoutMask, controller.ArrayDinMask);

  for(pe = 0;pe < simData.totalPE; pe++)
    {
        fprintf(simFiles.logFile, "Processor %d\n", pe);
	fprintf(simFiles.logFile, "operandA=%u operandB=%u operandC=%u cIn=%u alu=%u aCOut=%u ats=%u\n",
		LinearArray[pe].simWires.operandA, LinearArray[pe].simWires.operandB,
		LinearArray[pe].simWires.operandC, LinearArray[pe].simWires.cIn,
		LinearArray[pe].simWires.alu, LinearArray[pe].simWires.aCOut,
		LinearArray[pe].simWires.ats);   

	fprintf(simFiles.logFile, "cLatch=%u cCOut=%u cts=%u msb=%u minLatch=%u flag=%u address=%u multLo=%u\n",
		LinearArray[pe].simWires.cLatch, LinearArray[pe].simWires.cCOut, 
		LinearArray[pe].simWires.cts, LinearArray[pe].simWires.msb,
	        LinearArray[pe].simWires.minLatch, LinearArray[pe].simWires.flag, 
		LinearArray[pe].simWires.address, LinearArray[pe].simWires.multLo);   

	fprintf(simFiles.logFile, "eq=%u eqLatch=%u multHiLatch = %u result = %u bsLatch = %u mdrLatch = %u maskLatch = %u mask = %u\n\n",
		LinearArray[pe].simWires.eq, LinearArray[pe].simWires.eqLatch,
		LinearArray[pe].simWires.multHiLatch, LinearArray[pe].simWires.result,
		LinearArray[pe].simWires.bsLatch, LinearArray[pe].simWires.mdrLatch,
		LinearArray[pe].simWires.maskLatch, LinearArray[pe].simWires.mask); 

      if(pe == 0)
	{
	  fprintf(simFiles.logFile, "Left Register File\n");
	  for(j = 0;j < 32;j++)
	    {
	      if( leftmostRegFileFlag[j] == 0)
		{
		  fprintf(simFiles.logFile, "L[%d] = %d\n", j, leftmostRegFile[j]);
		}
	    }

	  fprintf(simFiles.logFile, "\n\nRight Register File\n");
	  
	  for(j = 0; j < 32; j++)
	    {
	      if( ( LinearArray[0].rightRegFileFlag[j]) == 0)
		{
		  fprintf(simFiles.logFile, "R[%d] = %d\n", j, 
			  (LinearArray[0].rightRegFile[j]) );
		}
	    }
	}

      else
	{
	  fprintf(simFiles.logFile, "\nLeft Register File\n");
	  for(j = 0; j < 32; j++)
	    {
	      if( LinearArray[pe-1].rightRegFileFlag[j] == 0)
		fprintf(simFiles.logFile, "L[%d] = %d ", j,
			LinearArray[pe-1].rightRegFile[j]); 
	    }

	  fprintf(simFiles.logFile, "\n\nRight Register File\n");

	  for(j = 0 ; j < 32; j++)
	    {
	      if( LinearArray[pe].rightRegFileFlag[j] == 0)
		{
		  fprintf(simFiles.logFile, "r[%d] = %d ", j,
			  LinearArray[pe].rightRegFile[j]);
		}
	    }
	}

      fprintf(simFiles.logFile, "\n\nSRAM\n");
      for(j = 0; j < 256; j++)
	{
	  if( (LinearArray[pe].sram[j]) != 0)
	    {
	      fprintf(simFiles.logFile, "sram[%d] = %d ", j, LinearArray[pe].sram[j]);
	    }
	}
  
      fprintf(simFiles.logFile, "\n\n");
    }

 }


/*************************************************************************
 * Array_Execute(void);
 *************************************************************************/

void Array_Execute(void)
{
  int count; 

  for (count = 0; count < simData.totalPE; count ++){
    OperandA(count);
    OperandC(count);
    OperandB(count);
    Mask(count);
    AddressGenerator(count);  
    ALU(count);
    Comparator(count);
    Multiplier(count);
    FlagSelector(count);
    ResultSelector(count);
    BitShifter(count);
    MaskFlagGenerator(count);
    SRAM(count);
    UpdateLatchValues(count);
  } 
  
  LeftRegFile();
  InnerRegFile();
  RightRegComm();
  
  totalInst++;
  
  /* compute Wired-Or before everything else because the result can be
   * placed on the flag bus. */
  controller.wor2 = controller.wor;
  controller.wor = 0;
  for (count = 0; count < simData.totalPE; count++) {
    WOR(count);
  }

#if DEBUG
  simFiles.logFile = fopen("log_file", "a+");
  LogFile();
  fclose(simFiles.logFile);
#endif

#if DEBUG
  simFiles.testFile = fopen("test_file", "a+");
  TestFile();
  fclose(simFiles.testFile);
#endif
}


void TestFile(void)
{
  int pe;	/* need not implement this function... osama*/
  extern void What_Inst(int);

  fprintf(simFiles.testFile,"Counter=%d\n", controller.pc);
  fprintf(simFiles.testFile,"Instr: %s\n", progmem[controller.pc]);
  fprintf(simFiles.testFile,"imm=%d, ", instruction.imm);
  fprintf(simFiles.testFile,"dest=%d, ", instruction.dest);
  fprintf(simFiles.testFile,"opC=%d, ", instruction.opC);
  fprintf(simFiles.testFile,"opA=%d, ", instruction.opA);
  fprintf(simFiles.testFile,"opB=%d, ", instruction.opB);
  fprintf(simFiles.testFile,"wr=%d, ", instruction.wr);
  fprintf(simFiles.testFile,"rd=%d, ", instruction.rd);
  fprintf(simFiles.testFile,"rm=%d, ", instruction.rm);
  fprintf(simFiles.testFile,"bit=%d", instruction.bit);
  fprintf(simFiles.testFile,"\nfb=%d, ", instruction.fb);
  fprintf(simFiles.testFile,"finv=%d, ", instruction.finv);
  fprintf(simFiles.testFile,"lc=%d, ", instruction.lc);
  fprintf(simFiles.testFile,"mp=%d, ", instruction.mp);
  fprintf(simFiles.testFile,"ci=%d, ", instruction.ci);
  fprintf(simFiles.testFile,"force=%d, ", instruction.force);

  for (pe = 0 ; pe < simData.totalPE ; pe++)
  {
    fprintf(simFiles.testFile, "Phase1 PE%d\noperandA=%3u operandB=%3u operandC=%3u cIn=%u alu=%3u aCOut=%u ats=%u\n", pe,
	    LinearArray[pe].simWires.operandA, LinearArray[pe].simWires.operandB,
	    LinearArray[pe].simWires.operandC, LinearArray[pe].simWires.cIn,
	    LinearArray[pe].simWires.alu, LinearArray[pe].simWires.aCOut,
	    LinearArray[pe].simWires.ats);   
    
    fprintf(simFiles.testFile, "cLatch=%u cCOut=%u cts=%u msb=%u minLatch=%u flag=%u address=%3u multLo=%3u\n",
	    LinearArray[pe].simWires.cLatch, LinearArray[pe].simWires.cCOut, 
	    LinearArray[pe].simWires.cts, LinearArray[pe].simWires.msb,
	    LinearArray[pe].simWires.minLatch,
	    LinearArray[pe].simWires.flag, LinearArray[pe].simWires.address,
	    LinearArray[pe].simWires.multLo);
    
    fprintf(simFiles.testFile, "eq=%u eqLatch=%u mask=%u wor=%u\n\n",
	    LinearArray[pe].simWires.eq, LinearArray[pe].simWires.eqLatch,
	    LinearArray[pe].simWires.mask, controller.wor); 
    
    fprintf(simFiles.testFile, "Phase1 PE%d\nflag=%u\n\n",
	    pe, LinearArray[pe].simWires.flag);
  }

  for (pe = 0 ; pe < simData.totalPE ; pe++)
  {
    fprintf(simFiles.testFile, "Phase2 PE%d\nresult=%3u bsLatch=%3u mdrLatch=%3u multHiLatch=%3u maskLatch=%u\n\n",
	    pe,
	    LinearArray[pe].simWires.result,
	    LinearArray[pe].simWires.bsLatch,
	    LinearArray[pe].simWires.mdrLatch,
	    LinearArray[pe].simWires.multHiLatch,
	    LinearArray[pe].simWires.maskLatch); 
    fprintf(simFiles.testFile, "Phase2 PE%d\nresult=%3u bsLatch=%3u\n",
	    pe, LinearArray[pe].simWires.result,
	    LinearArray[pe].simWires.bsLatch);
  }

  fprintf(simFiles.testFile, "\n");
}
