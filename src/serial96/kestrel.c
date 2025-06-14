/**Redo This************************************************************************ 
  This is a serial program to simulate the Kestrel parallel architecture.
  Kestrel is a SIMD linear systolic array controlled by a microinstruction
  sequencer.  In this program, the MasPar's PE array acts as the linear array
  of processors, while the MasPar's ACU serves the function of the
  microinstruction sequencer.

   Created:  December 12, 1994
   Written by:  Jeffrey D. Hirschberg
   Modified:  
   April, 1995, updated model to incorporate architectural changes.  
   August, 1995, added single step control and breakpoints.
   February, 1996, updated architectural model.
   July, 1996, updated architectural model and major overhaul of simulator
   structure to make it simpler.
   July, 1997, Brought up to date by Osama Salem.
   Sept., 1997, Added debugger interface by Justin Meyer
   sept.  1997, main function and loadProgMem function changed to better 
   simulate final version of board by Jennifer Leech.
   Dec. 1998 jleech deleted functions dealing with external input and
   output files as these will only be dealt with by the run time environment.
   Then, what was previously called "stageIn" was renamed "Qin" etc.
   Jan 1998 got rid of breakpoint function...finished PCIchip fn.
      programmem array is now gone
      loadprogmem deleted, dealwithit deleted
      loop function deleted--the function of loop will now be handled in the
      controller.
*****************************************************************************/
#define DEBUG  0


/*****************************************************************************
  Include necessary header files
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "kestrel.h"
#include "kesboard.h"
#include "controller.h"

/*****************************************************************************
  Declare global variables, structures and arrays
*****************************************************************************/

struct stat filedesStruct;

/* Files used in the simulation*/
files simFiles;

/* Read and Write Pipes */
int Read_Pipe, Write_Pipe = 0;
int filedesc[2];

/* Pointer to Linear Array */
PE_ptr LinearArray;

/* Left Most Register File for Processor Element Number 0 */
unsigned char leftmostRegFile[REG_FILE_SIZE];
unsigned char leftmostRegFileFlag[REG_FILE_SIZE];  /* Has this register been used ??? */
char IRQ[9] = "FFFFFFFF\0"; 

/* Counters and flags */
data simData;

/* osama. total number of instructions */
int totalInst = 0;

int total_instr_executed = 0;

/*****************************************************************************
   Declare functions. 
*****************************************************************************/

void Interrupt( void ) {}

/*****************************************************************************
ExaminePE   debugging function
*****************************************************************************//*
void ExaminePE(int pe)
{
  int j;

  printf("Processor %d\n", pe);
  printf("Current Instruction Type Number %d\n",instruction.num);

  printf("operandA=%u operandB=%u operandC=%u cIn=%u alu=%u aCOut=%u ats=%u\n",
	 (LinearArray+pe)->simWires.operandA, (LinearArray+pe)->simWires.operandB,
	 (LinearArray+pe)->simWires.operandC, (LinearArray+pe)->simWires.cIn,
	 (LinearArray+pe)->simWires.alu, (LinearArray+pe)->simWires.aCOut,
	 (LinearArray+pe)->simWires.ats);		  

  printf("cLatch=%u cCOut=%u cts=%u msb=%u min=%u minLatch=%u flag=%u address=%u multLo=%u\n",
	 (LinearArray+pe)->simWires.cLatch, (LinearArray+pe)->simWires.cCOut,
	 (LinearArray+pe)->simWires.cts, (LinearArray+pe)->simWires.msb, (LinearArray+pe)->simWires.min,
	 (LinearArray+pe)->simWires.minLatch, (LinearArray+pe)->simWires.flag,
	 (LinearArray+pe)->simWires.address, (LinearArray+pe)->simWires.multLo);   

  printf("eq=%u eqLatch=%u multHiLatch=%u result=%u bsLatch=%u mdrLatch=%u mask=%u\n\n",
	 (LinearArray+pe)->simWires.eq, (LinearArray+pe)->simWires.eqLatch,
	 (LinearArray+pe)->simWires.multHiLatch, (LinearArray+pe)->simWires.result,
	 (LinearArray+pe)->simWires.bsLatch, (LinearArray+pe)->simWires.mdrLatch,
	 (LinearArray+pe)->simWires.mask);

  if(pe == 0)
    {
      printf("Left Register File\n");
      for(j = 0; j < 32; j++)
	{
	  if( leftmostRegFileFlag[j] == 0)
	    {
      printf("L[%d] = %d ", j,leftmostRegFile[j] );
	    }
	}

      printf("\n\nRight Register File\n");
      
      for(j = 0; j < 32; j++)
	{
	  if( ((LinearArray+pe)->rightRegFileFlag[j]) == 0)
	  printf("R[%d] = %d ", j, (LinearArray+pe)->rightRegFile[j] );
	}
    }

  else
    {
      printf("\nLeft Register File\n");
      for(j = 0; j < 32; j++)
	{
	  if( (LinearArray + pe - 1)->rightRegFileFlag[j] == 0)
	  printf("L[%d] = %d ", j, (LinearArray+pe - 1)->rightRegFile[j]);
	}

      printf("\n\nRight Register File\n");

      for(j = 0 ; j < 32; j++)
	{
	  if( (LinearArray+pe)->rightRegFileFlag[j] == 0)
	  printf("r[%d] = %d ", j, (LinearArray+pe)->rightRegFile[j]);
	}
    }

  printf("\n\nSRAM\n");
  for(j = 0; j < 256; j++)
    {
      if(( (LinearArray+pe)->sram[j]) != 0)
	{
	  printf("sram[%d] = %d ", j, (LinearArray+pe)->sram[j]);
	}
    }
  
  printf("\nPress the <erter> key to continue...");
  getchar();
  fflush(stdin);

  printf("\n");
}*/



/***************************************/
void Create_Linear_Array(void)
{
  int count1,count2;

  /* Allocate Memory for the PE Linear Array */

  LinearArray = (PE_ptr) calloc(simData.totalPE,sizeof(PE));


  /* Intialize SSR's to non-zero number */

  for (count1 = 0; count1 < REG_FILE_SIZE; count1++)
    {
      for(count2 = 0; count2 < simData.totalPE; count2++)
	(LinearArray+count2)->rightRegFileFlag[count1] = 255; 
      
      leftmostRegFileFlag[count1] = 255;
    }
}
/***************************************/


/**************************************************************************

  PCIchip();
  this function simulates the action of the PCI chip on the Kestrel board.
  It takes 32 bits of address and 32 bits of data piped from the kestrel
  interface, currently called kdb, and directs the data accordingly.

  -Jennifer Leech
  *************************************************************************/

 
/* this function communicates with the parent program by accepting and
   receiving strings of 9 characters which represent 32 bits in hex
   plus a null character. */
void PCIchip(int blocking)
{  
  pcidata_ptr pciInfo;
  int out_bytes = 9, in_int;
  char address[9];
  char in_data[9];  /* 8 hex digits plus a null character  */    
  char out_data[9]={0};
  unsigned char io_char;
  int emptypipe = 0;
  unsigned long conv_int;

  fstat( Read_Pipe, &filedesStruct ); 
  emptypipe = filedesStruct.st_size;

  /* read stuff from the FIFO if there is something there or we want
   * to block (go to sleep waiting for input from RTE) */
  if( emptypipe != 0 || blocking)
    {
      
      read( Read_Pipe, address, sizeof(address));
      if (address[8] != 0) {
	fprintf(stderr, "kestrel sim: FATAL ERROR: PCIChip read bad data.\n");
	fflush(stderr);
	exit(99);
      }

      conv_int = strtoul(address, NULL, 16);   /* converting string of hex to int*/

#if DEBUG > 1
      fprintf( stderr, "sim: address is \t\t[%s]\n", address );
      fflush(stderr);
#endif

      pciInfo = Process_PCI_data( conv_int ); /* this function translates the hex
						 address to something meaningful
						 to the program */
      if( pciInfo->read_or_write == 1 ){
	read( Read_Pipe, in_data, sizeof(in_data));
	if (in_data[8] != 0) {
	  fprintf(stderr, "kestrel sim: FATAL ERROR: PCIChip read bad data.\n");
	  fflush(stderr);
	  exit(99);
	}
#if DEBUG > 1
	fprintf(stderr, "sim: data is \t\t[%s]\n", in_data );
	fflush(stderr);
#endif
      }
      
      switch( pciInfo->address )
	{
	case STATUS_REG_ADDRESS: 
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      /* Get_StatusReg returns a string of the proper length */
	      write( Write_Pipe, Get_StatusReg(), out_bytes);
#if DEBUG	      
	      fprintf(stderr, "sim: status register read: %s\n",
		      Get_StatusReg());
	      fflush(stderr);
#endif
	    }
	  else                           /* writing to the reg...*/
	    {                            /* this actually should never happen */
	      fprintf(stderr, "kestrel sim: Attempted write to status register.\n");
	      fflush(stderr);
	    }
	  break;
	  
	case QIN_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      fprintf(stderr, "kestrel sim: Program attempted read of QIn\n");
	      fflush(stderr);
	    }
	  else                           /* writing to the reg...*/
	    { 
	      QInPutData(strtol( in_data, NULL, 16 ));
#if DEBUG
	      fprintf(stderr, "sim: writing [%ld] to Qin[%d]\n", 
		      strtol( in_data, NULL, 16 ), QInBytes);
	      fflush(stderr);
#endif
	    }
	  break;

	case QOUT_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      if (QOutGetData(&io_char)) {
		sprintf( out_data, "%08x", io_char);
		write( Write_Pipe, out_data, out_bytes );
#if DEBUG
		fprintf( stderr, "sim: read from Qout: [%s]\n", out_data); 
		fflush(stderr);
#endif
	      } 
	    }
	  else                           /* writing to the reg...*/
	    {   
	      fprintf(stderr, "kestrel sim: Program attempted write to QOut.\n");
	      fflush(stderr);
	    }
	  break;
	  
	case CMD_REG_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      fprintf(stderr, "kestrel sim: Attempted read of command register.\n");
	      fflush(stderr);
	    } 
	  else                           /* writing to the reg...*/
	    {
#if DEBUG
	      fprintf( stderr, "kestrel sim: the cmd_reg_address has just been" );
	      fprintf( stderr, " written to: \t\t[%s]\n", in_data );
	      fflush(stderr);
#endif
	      conv_int = strtol( in_data, NULL, 16 );
	      OldCommandReg = CommandReg;
	      CommandReg = conv_int;  	  

	      /* terminate simulator on 1->0 transition of msb of command register */
	      if ((OldCommandReg & CNTRBIT_FPGA_RESET) == CNTRBIT_FPGA_RESET &&
		  (CommandReg & CNTRBIT_FPGA_RESET) == 0) {
    fprintf(stderr, "\nkestrel sim: serial simulator executed %d instructions.\n", simData.instCount); 
		fprintf(stderr, "kestrel sim: serial simulator terminating.\n");
		fflush(stderr);
		exit (0);
	      }
	    }
	  break;  
	  
	case T1_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    { 
	      strcpy(out_data, Read_T_One());
	      write( Write_Pipe, out_data, out_bytes );
	    }
	  else                           /* writing to the reg...*/
	    {
#if DEBUG > 1
	      fprintf(stderr, "sim: T1_ADDRESS has \t\t[%s]\n", in_data );
	      fflush(stderr);
#endif
	      Write_T_One(in_data);
	    }
	  break;
	case T2_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      strcpy(out_data, Read_T_Two());
	      write( Write_Pipe, out_data, out_bytes );
	    }
	  else                           /* writing to the reg...*/
	    {
#if DEBUG > 1
	      fprintf(stderr, "sim: T2_ADDRESS has \t\t[%s]\n", in_data );
	      fflush(stderr);
#endif
	      Write_T_Two(in_data);
	    }
	  break;
	case T3_ADDRESS:
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      strcpy(out_data, Read_T_Three());
	      write( Write_Pipe, out_data, out_bytes );
	    }
	  else                           /* writing to the reg...*/
	    {   
#if DEBUG > 1
	      fprintf(stderr, "sim: T3_ADDRESS has \t\t[%s]\n", in_data );
	      fflush(stderr);
#endif
	      Write_T_Three(in_data);
	    }
	  break;

	case PROG_FIFO_ADDRESS: 
	  if( !(pciInfo->read_or_write) )   /* 0 = read, 1 = write */
	    {
	      fprintf(stderr, "kestrel sim: Attempted read of Qin in PROG_FIFO mode.\n");
	      fflush(stderr);
	    } 
	  else                           /* writing to the reg...*/
	    {
	      in_int = strtol( in_data, NULL, 16 );
	      WriteQueueLevel(MODIFY_LEVEL_QIN, &QInAlmostEmptyLevel, 
			      &WriteQInLevelPtr, in_int);
	      Status_Register.i_almost_empty = IsQInAlmostEmpty();
	    }
	  break;

	case READ_QOUT:
	  {
	    int length, i;
	    unsigned char *buffer;

	    length = strtol(in_data, NULL, 16);
	    buffer = malloc(length);
	    for (i = 0; i < length; i++) {
	      QOutGetData(&io_char);
	      buffer[i] = io_char;
	    }
	    write(Write_Pipe, buffer, length);
	    free(buffer);
	  }

	  break;
	  
	}
    }
}


/***************************************************************************
  getOpts();

  created by Jennifer Leech on 12/16/97
  I simply moved the command line option checking into a function to make
  main nicer...really, it should probably go since this will no longer
  be interactive...
  **************************************************************************/

void Get_Opts( int argc, char** argv)
{

  /* string "64", "-r", sim_read_end, "-w", sim_write_end );*/
  if(argc < 4)
    {
      fprintf(stderr, "\nThis simulator must be invoked from within Kasm controller...\n");
      fprintf(stderr, "\nusage: serial [pe] [-r read pipe]\n[-w write pipe]\n");
      fprintf(stderr, "pe         -- number of PEs in simulated array\n");
      fprintf(stderr, "-r         -- pipe read end\n");
      fprintf(stderr, "-w         -- pipe write end\n");
      /*      fprintf(stderr, "-l log     -- file log used for log of simulation events\n"); 
       */
      exit(1);
    }
  
  simData.totalPE = atoi(argv[1]);
  
  /*  pipe(filedesc);
   */
  Read_Pipe = argv[3][0];
  Write_Pipe = argv[5][0];
   
}


void Dump_PE_Reg1(void)
{
  FILE* debug;
  int j;
  int i = 0;
  debug = fopen("reg_dump", "a+");
  fprintf(debug,"\n\n\nREGISTER DUMP @ PC = %d\n",controller.pc);
  fprintf(debug,"==========================================\n");
  while(i < simData.totalPE)
    {
      fprintf(debug,"sim: Contents of PE %d [ ", i);
       for(j = 0;j < 31;j++)
	 {
	
	   fprintf(debug,"(%d):%d ", j, (LinearArray+i)->rightRegFile[j]);
	 }
      
       i++;
       fprintf(debug, "]\n");
    }

  fclose(debug);
}



/***************************************************************************** 
  main -- Here is where the "microcontroller and board" are initialized.

  1.  Load the staging memory with the data to be processed during
      simulation.

  2.  Load the program memory with the program to simulated.

  3.  Dynamically allocate memory for the PE Linear Array


*****************************************************************************/
int main(int argc, char **argv)
{
  int state;
  int blocking;   /* do we want blocking FIFO reads? depends on state
		   * of controller: no for RUNNING, yes for STOP and SINGLE */
  int load_toggle_changed, single_step_instr = 0;
  int state_was_stop = 1;

  /* Initialzie variables */
  simData.output = 0;
  simData.log = 0;
  simData.instCount = 0;

  Get_Opts( argc, argv );
 
  Init_Board();
  Init_Controller();
  Create_Linear_Array();

  /* set blocking on: we wait for input on the FIFO from the RTE */
  blocking = 1;

  while(1) {

    PCIchip(blocking); 
   	  
    /* do a read of QIn if the QIN_LOAD bit in the commands register goes
     * from 0 to 1 */
    if (((OldCommandReg & CNTRBIT_QIN_LOAD) == 0) &&
	((CommandReg & CNTRBIT_QIN_LOAD) == CNTRBIT_QIN_LOAD)) {
      unsigned char junk; 
      QInGetData(&junk);
    }

    /* next we check the cmd register to see how the control bits are set.  
       Through a switch, a function will be called that will simulate the 
       controller entering the state specified by the command register. */
    state = CommandReg & (CNTRBIT_RUN|CNTRBIT_SINGLE|CNTRBIT_DIAG);

    /* detect a change in the LOAD_TOGGLE bit */
    load_toggle_changed = (OldCommandReg & CNTRBIT_LOAD_TOGGLE) ^
                          (CommandReg & CNTRBIT_LOAD_TOGGLE);
    if (load_toggle_changed) {
      OldCommandReg = CommandReg;
    }

    switch( state )   /* states defined in kesboard.h */
      {
      case CNTR_MODE_STOP: /* not doing anything */
#if DEBUG 
	fprintf(stderr, "STOP");
	fflush(stderr);
#endif
	blocking = 1;
	single_step_instr = 0;
	controller.execution_halted = 0;
	Status_Register.break_bit = 0;
	break;

      case CNTR_MODE_RUN:       /* execute instructions. */
	if (controller.execution_halted) {
	  break;
	}
	simData.instCount++; 
	/* when in execute mode, first, a line of hex will need to be 
	 * copied form program mem to the transcievers, then interpret 
	 * inst will need to be called, then execute can be called.
	 */
#if DEBUG > 1
	fprintf(stderr, "RUN: [%d] ",controller.pc);
	fflush(stderr);
#else
#if 0
	/* spinning thing to let the user know kestrel is running */
	putchar('\b');
	switch(total_instr_executed % 4)  
 	  {  
 	  case 0:  
 	    putchar('-');  
 	    break;  
 	  case 1:  
 	    putchar('\\'); 
 	    break;  
 	  case 2: 
 	    putchar('|'); 
 	    break; 
 	  case 3: 
 	    putchar('/'); 
 	    break; 
 	  } 
#endif
#endif

	strcpy( Transceiver, progmem[controller.pc]); 
	/* this puts the next instruction in the transceiver */
	if (!Execute_Instruction(state_was_stop, state)) {
	  blocking = 1;
	  break;
	}

 	total_instr_executed++; 

#if DEBUG > 1	
	Dump_PE_Reg1();
#endif	

	blocking = 0;
	/* if we changed state to STOP, then blocking will be changed
	 * to TRUE above */
	break;

      case CNTR_MODE_SINGLE:
	if (controller.execution_halted || single_step_instr) {
	  break;
	}

	/* single step execution-same as execute, except it 
	 * waits for input from the pipe before it will go on.
	 */
#if DEBUG
	fprintf(stderr, "SINGLE");
	fflush(stderr);
#endif
	/* this puts the next instruction in the transceiver */
	strcpy( Transceiver, progmem[controller.pc]);  
	Execute_Instruction(1, state);

	blocking = 1;
	single_step_instr = 1;
	break;

      case CNTR_MODE_READ_INS:
	break;

      case CNTR_MODE_LOAD_INS:       /* loading instruction memory */
	if (load_toggle_changed) {
	  /* this is to be used after all three transcievers have been written to
	   * and the entire instruction is ready to be loaded to program mem.
	   * the run-time environment will have to keep track of these conditions.
	   *
	   * host must initialize PC to location of first instruction.
	   */
	  controller.pc = (controller.pc + 1) % PROG_SIZE;

#if DEBUG
	  fprintf( stderr, "kestrel sim: writing instruction to programmem.\n" );
	  fprintf( stderr, "kestrel sim: controller.pc == %d, instr == %s\n", 
		   controller.pc, Transceiver );
#endif
	  strcpy( progmem[controller.pc], Transceiver );

#if DEBUG > 1
	  fprintf( stderr, "sim: \t\t\t progmem holds [%s]\n",  
		   progmem[controller.pc] );
#endif
	}
	blocking = 1;
	break;

      case CNTR_MODE_PROG_FIFO:
	if (load_toggle_changed) {
	  /* executes instruction loaded into transceiver */
#if DEBUG
	  fprintf(stderr, "kestrel sim: Executing %s in program fifo mode.\n",
		  Transceiver);
	  fflush(stderr);
#endif
	  Execute_Instruction(1, state);
	}
	blocking = 1;
	break;

      case CNTR_MODE_DIAG:
	if (load_toggle_changed) {
	  /* executes instruction loaded into transceiver */
#if DEBUG
	  fprintf(stderr, "kestrel sim: Executing %s in diagnostic mode.\n",
		  Transceiver);
	  fflush(stderr);
#endif
	  Execute_Instruction(1, state);
	}
	blocking = 1;
	break;

      default:
	fprintf( stderr, "sim: default case of main\n" );
	break;
      }

    if (state != CNTR_MODE_STOP) {
      state_was_stop = 0;
    } else {
      state_was_stop = 1;
    }
  }

  fprintf( stderr, "sim: should never get here... program is finished." );

  return (0);
}

void TriggerIRQ(void)
{
  write( Write_Pipe, IRQ,  9 );
}
