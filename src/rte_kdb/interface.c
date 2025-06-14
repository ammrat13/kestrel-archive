/* $Id: interface.c,v 1.20 2001/06/13 22:47:56 ericp Exp $ */
/* 
 * Kestrel Run Time Environment
 * Copyright (C) 1998  Regents of the University of California.
 *
 * interface.c - routines for basic, low level comminication
 *		with the kestrel board/simulator.
 */

#include "globals.h"
#include "pe.h"
#include "interface.h"
#include "program.h"
#include "code.h"
#include "getstate.h"
#include "queues.h"
#include "kdb.h"

#define DEBUG 0

int CommandReg = 0;
int GotIRQDuringRead = 0;

char *Put_in_Pipe(int address, int rowr)
{
  static char buf[20];
  if (rowr == READ)
    {
      address = address | READ;
      sprintf(buf,"%08x", address);
    }
  else if (rowr == WRITE)
    {
      address = address | WRITE;
      sprintf(buf,"%08x", address);
    }
  else 
    {
      sprintf(buf,"%08x", address);
    }
  return buf;
}

/* 
 * Function   : WriteDataToKestrel, ReadDataFromKestrel
 * Purpose    : Tramsit/recieve one number of the kestrel
 *		board/simulator.  The item is sent as a 8 byte
 *		hexidecimal ascii string with a numm terminator.
 * Parameters : value - value to send
 * Returns    : value read from board/simulator
 * Notes      : 
 */
void WriteDataToKestrel(int value)
{
  char buf[10];
  sprintf(buf, "%08x", value);
  KestrelWrite((unsigned char *)buf, 9);
}
int ReadDataFromKestrel(void)
{
  unsigned char buf[10];
  
  while (1) {
    KestrelRead(buf, PIPE_WIDTH);
    if (!strcmp((char *)buf, IRQ)) {
      GotIRQDuringRead = 1;
    } else {
      break;
    }
  }
  return strtoul((char *)buf, NULL, 16);
}


/* 
 * Function   : KestrelRead/KestrelWrite
 * Purpose    : read/write a block of data from/to the kestrel
 *		board/simulator.  The transfer is forced to continue
 *		until all bytes have been recieved/sent.
 * Parameters : storage - block of memory to recieve/send
 *		bytes_toread/bytes_to_write - size of transfer
 * Returns    : 1 - success, 0 - doh!
 * Notes      : 
 */
int KestrelRead(unsigned char *storage, int bytes_to_read)
{
    int total, current;

    current = 0, total = 0;

    while (total < bytes_to_read) {
      current = read(read_from_kestrel, &storage[total], bytes_to_read - total);
      if (current == -1) {
	return 0;
      }
      total += current;
    }
    return 1;
}
int KestrelWrite(unsigned char *storage, int bytes_to_write)
{
  int total, current;

  current = 0, total = 0;

  while(total < bytes_to_write) {
    current = write(write_to_kestrel, &storage[total], bytes_to_write - total);
    if (current == -1) {
      ErrorPrint("KestrelWrite", "Error writing to board/simulator.\n");
      return 0;
    }
    total += current;
  }
  return 1;
}

/* 
 * Function   : GetStatusRegister
 * Purpose    : Read the status register and modify some bits
 *		(invert active low signals, and zero unused bits).
 * Parameters : nothing
 * Returns    : contents of status register
 * Notes      : 
 */
unsigned int GetStatusRegister(void)
{
  int status;
 
  write(write_to_kestrel, Put_in_Pipe(STATUS_REG_ADDRESS,READ),	PIPE_WIDTH);
  status = ReadDataFromKestrel();

  /* invert polarity on these bits for RTE (active low signals) */
  status = status ^ (STATUS_REG_QOUT_FULL|STATUS_REG_QOUT_ALMOST_FULL|STATUS_REG_QIN_EMPTY|STATUS_REG_QIN_ALMOST_EMPTY);

  /* zero these bits (there not used) */
  status &= ~(STATUS_REG_DONE|STATUS_REG_LOAD_DONE|STATUS_REG_O_STATUS_2);

  return status;
}

/* 
 * Function   : PutCommandReg
 * Purpose    : Write a value to the command register. Tracks the
 *		load_toggle bit to keep it the same unless
 *		specifically toggled
 * Parameters : mode - operating mode or bits.
 *		load_toggle: 1 flip load_toggle, 0: perserve, -1: reset to 0
 * Returns    : 
 * Notes      : 
 */
void PutCommandReg(int mode, int load_toggle)
{
  int new_cmd_reg = 0;
  
  if (load_toggle == -1) {
    CommandReg &= ~CNTRBIT_LOAD_TOGGLE;
  }
  if (load_toggle == 1) {
    if (!(CommandReg & CNTRBIT_LOAD_TOGGLE)) {
      new_cmd_reg |= CNTRBIT_LOAD_TOGGLE;
    }
  } else {
    new_cmd_reg = CommandReg & CNTRBIT_LOAD_TOGGLE;
  }
  new_cmd_reg |= (mode | CNTRBIT_BOARD_ENABLE_MASK);

  new_cmd_reg |= CNTRBIT_ALL_ENABLES;

  write(write_to_kestrel, Put_in_Pipe(CMD_REG_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(new_cmd_reg,0), PIPE_WIDTH);
  CommandReg = new_cmd_reg;
}

/* 
 * Function   : DecodeStatusRegister
 * Purpose    : Display a textual message depicting the contents
 *		of the kestrel controller status register
 * Parameters : value - value of status register
 * Returns    : nothing
 * Notes      : 
 */
void DecodeStatusRegister(int value)
{
  int shift, mask;
  char *string;

  for (shift = 0; shift < 16; shift++) {
    mask = 1 << shift;

    if (!(value & mask)) {
      continue;
    }

    switch (value & mask) {
    case STATUS_REG_COUNTER_OVERFLOW :
      string = "COUNTER STACK OVERFLOW";
      break;

    case STATUS_REG_COUNTER_UNDERFLOW :
      string = "COUNTER STACK UNDERFLOW";
      break;

    case STATUS_REG_PC_OVERFLOW :
      string = "PC STACK OVERFLOW";
      break;

    case STATUS_REG_PC_UNDERFLOW:
      string = "PC STACK UNDERFLOW";
      break;

    case STATUS_REG_QOUT_FULL:
      string = "QOUT FULL";
      break;

    case STATUS_REG_QOUT_ALMOST_FULL:
      string = "QOUT ALMOST FULL";
      break;
     
    case STATUS_REG_QIN_EMPTY :
      string = "QIN EMPTY";
      break;

    case STATUS_REG_QIN_ALMOST_EMPTY :
      string = "QIN ALMOST EMPTY";
      break;
    
    case STATUS_REG_BREAKPOINT :
      string = "BREAK POINT";
      break;

    case STATUS_REG_VALID_NEXT_DATA :
      string = "CNTR QIN EMPTY";
      break;
     
    default :
      sprintf(print_msg, "UNKNOWN: %x", mask);
      string = print_msg;
      break;
    }
    printf("<%s>", string);
  }

  printf ("\n");
  fflush(stdout);
}


/* 
 * Function   : ProcessStatusRegister
 * Purpose    : Respond to conditions on the kestrel board
 *		based on the bits in the status register. This includings
 *		servicing the data queues, handling breakpoints, and
 *		noting error conditions (stack over/underflow).
 * Parameters : program - program currently executing
 *		state - current execution state of program
 * Returns    : 1 - end of program reached
 *		0 - error condition
 *		-1 - nothing interesting happened
 * Notes      : Queue almost full/empty processing is not done because
 *		the runtime environment can't keep up with the board over
 *		a network connection, and correct handling involves
 *		further complications tha can only be dealt with effectively
 *		by the server.
 */
int ProcessStatusRegister(UserProgram *program, int state)
{
  unsigned int value;
  int error = 0, stop = 0, ret, pc;

  value = GetStatusRegister();
  if (user_show_irq) {
    DecodeStatusRegister(value);
  }
  error = 0;

  if (value & STATUS_REG_COUNTER_OVERFLOW) {
    ScreenPrint("Program Error: Counter stack overflow\n");
    error ++;
    stop = 1;
  }
  if (value & STATUS_REG_COUNTER_UNDERFLOW) {
    ScreenPrint("Program Error: Counter stack underflow\n");
    error ++;
    stop = 1;
  }
  if (value & STATUS_REG_PC_OVERFLOW) {
    ScreenPrint("Program Error: PC stack overflow\n");
    error ++;
    stop = 1;
  }
  if (value & STATUS_REG_PC_UNDERFLOW) {
    ScreenPrint("Program Error: PC stack underflow\n");
    error ++;
    stop = 1;
  }

  if (value & STATUS_REG_QOUT_FULL) {
    PutCommandReg(CNTR_MODE_STOP, 0);
    if (!ReadQout(program, FIFO_SERVE_FULL)) {
      error ++;
      stop = 1;
    }
    if (!(value & STATUS_REG_BREAKPOINT) && state == CNTR_MODE_RUN) {
      if (!(value & STATUS_REG_VALID_NEXT_DATA && !(value & STATUS_REG_BREAKPOINT))) {
	PutCommandReg(CNTR_MODE_RUN, 0);
      }
    }
#if 0
  } else if (value & STATUS_REG_QOUT_ALMOST_FULL) {
    if (!ReadQout(program, FIFO_SERVE_ALMOST)) {
      error ++;
      stop = 1;
    }
#endif
  }

  if (value & STATUS_REG_VALID_NEXT_DATA ) {
    PutCommandReg(CNTR_MODE_STOP, 0);
    if (!(value & STATUS_REG_QIN_EMPTY)) {
      sprintf(print_msg, "FATAL inconsistency in status register.\n");
      ErrorPrint("ProcessStatusRegister", print_msg);
      sprintf(print_msg, "!QIN_EMPTY && VALID_NEXT_DATA == TRUE.\n");
      ErrorPrint("ProcessStatusRegister", print_msg);
      QuitKdb();
    }

    if (!LoadQin(program, FIFO_SERVE_FULL)) {
      error ++;
      stop = 1;
    }
    if (!(value & STATUS_REG_BREAKPOINT) && state == CNTR_MODE_RUN) { 
      PutCommandReg(CNTR_MODE_RUN, 0);
    }
#if 0
  } else if (value & STATUS_REG_QIN_ALMOST_EMPTY && !(value & STATUS_REG_BREAKPOINT)  ) {
    if (!LoadQin(program, FIFO_SERVE_ALMOST)) {
      error++;
      stop = 1;
    }
#endif
  }

  if (value & STATUS_REG_BREAKPOINT) {    
    pc = GetControllerPC();
    if (pc < program->code_offset || pc >= program->code_offset + program->code_instr) {
      sprintf(print_msg, "Breakpoint detected at invalid instruction %d",pc);
      ErrorPrint("ProcessStatusRegister", print_msg);
      error++;
      stop = 1;
      ret = 0;
    } else {
      if (!strcmp(program->code_instrptr[pc - program->code_offset], END_OF_PROGRAM)) {
	sprintf(print_msg, "end of program reached at %d\n", pc);
	ScreenPrint(print_msg);
	stop = 1;
	ret = 1;
      } else {
	sprintf(print_msg, "breakpoint reached at %d\n", pc);
	ScreenPrint(print_msg);
	DisplayInstruction(program, pc);
	stop = 1;
	ret = 0;
      }
    }
  }

  if (stop) {
    if (error) {
      pc = GetControllerPC();
      sprintf(print_msg, "program execution stopped on %d error(s) at %d.\n",
	      error, pc);
      ScreenPrint(print_msg);
      DisplayInstruction(program, pc);
      ret = 0;
    } 
  } else {
    ret = -1;
  }
  return ret;
}

/* 
 * Function   : RunningKestrel
 * Purpose    : This routine is in control while a kestrel program
 *		is running on the board/simulator. It handles interrupts
 *		and terminates when an error or an end of program is reached.
 * Parameters : program - current program
 *		action - ACTION_RUN, program is running, otherwise
 *			just handle any pending status register condtions.
 * Returns    : 1 - end of program
 *		0 - error
 * Notes      : 
 */
int RunningKestrel(UserProgram *program, int action)
{
  unsigned char buf[9];
  int ret;

  InvalidateState(program);

  if (action == ACTION_RUN) {
    while (1) {
      if (GotIRQDuringRead == 1) {
	GotIRQDuringRead = 0;
	if (user_show_irq) {
	  ScreenPrint("Delayed IRQ signal: ");
	}
	ret = ProcessStatusRegister(program, CNTR_MODE_RUN);
	if (ret == 1) {
	  return (1);
	} else if (ret == 0) {
	  return (0);
	}
      } else {
	write(write_to_kestrel, Put_in_Pipe(ENABLE_INTERRUPTS,WRITE),PIPE_WIDTH);
	write(write_to_kestrel, Put_in_Pipe(0x00000000,0),PIPE_WIDTH);
	fflush(stdout);
	KestrelRead(buf, PIPE_WIDTH);
	if(!strcmp((char *)buf, IRQ)) {
	  if (user_show_irq) {
	    ScreenPrint("Received IRQ signal: ");
	  }
	  ret = ProcessStatusRegister(program, CNTR_MODE_RUN);
	  if (ret == 1) {
	    return (1);
	  } else if (ret == 0) {
	    return (0);
	  }
	} else {
	  sprintf(print_msg, "Recieved data %s for IRQ\n", buf);
	  ErrorPrint("RunningKestrel", print_msg);
	}
      }
    }
  } else {
    ScreenPrint("Status Register: ");
    return ProcessStatusRegister(program, CNTR_MODE_SINGLE);
  }
}

/* 
 * Function   : CheckProgrammedFPGA
 * Purpose    : Check the DONE bit of the status register to ensure
 *		the Xilinx FPGA is programmed. Exits() if unprogrammed.
 * Parameters : none
 * Returns    : nothing.
 * Notes      : 
 */
void CheckProgrammedFPGA(void)
{
  int status;
  unsigned char buf[10];
  int ret;
  write(write_to_kestrel, Put_in_Pipe(STATUS_REG_ADDRESS,READ),	PIPE_WIDTH);
  ret = KestrelRead(buf, PIPE_WIDTH);
  if (ret == 0) {
    ErrorPrint("CheckProgrammedFPGA", "read failed (bad connection or connection refused).");
    QuitKdb();
  }
  status = strtoul((char *)buf, NULL, 16);
  if ((status & STATUS_REG_DONE) == 0) {
    ErrorPrint("HARDWARE CHECK", "FPGA is unprogrammed (DONE is not high).");
    QuitKdb();
  }
} 

/* 
 * Function   : InitializeBoard
 * Purpose    : Prepare the kestrel board for program execution
 * Parameters : none
 * Returns    : nothing
 * Notes      : 
 */
void InitializeBoard(void) 
{
  if (have_board) {
    CheckProgrammedFPGA();
  }

  /* initialize command register and next qin valid flag */
  PutCommandReg(CNTR_MODE_STOP, 0);
  PutCommandReg(CNTRBIT_QIN_LOAD, 0);
  PutCommandReg(CNTRBIT_QIN_LOAD, 0);
  PutCommandReg(CNTR_MODE_STOP, 0);
} 

/* 
 * Function   : Kill_Kestrel 
 * Purpose    : This function is used to cause the serial simulator
 *		to exit().  When used with the board, it causes
 *		a local reset.
 * Parameters : none
 * Returns    : nothing
 * Notes      : 
 */
void Kill_Kestrel(void)
{
  write(write_to_kestrel, Put_in_Pipe(CMD_REG_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(CNTR_MODE_TERMINATE,0), PIPE_WIDTH);
}


/* 
 * Function   : SetControllerPC, GetControllerPC
 * Purpose    : Low level functions to manipulate the FPGA program
 *		counter by executing controller instructions in
 *		diagnostic mode.
 * Parameters : new_pc - new program counter
 * Returns    : current program counter
 * Notes      : 
 */
void SetControllerPC(int new_pc)
{
  char *instr;

  PutCommandReg(CNTR_MODE_STOP, 0);

  /* diagnostic mode runs the instruction in tranceivers */
  PutCommandReg(CNTR_MODE_DIAG, 0);

  /* make instruction to load new PC */
  instr = MakeControllerSetPCInstr(new_pc);
  WriteInstrToTransceivers(instr);

  /* run the instruction in the transceivers */
  PutCommandReg(CNTR_MODE_DIAG, 1);

  PutCommandReg(CNTR_MODE_STOP, 0);
}
int GetControllerPC(void)
{
  char *instr;
  int old_pc = 0;

  PutCommandReg(CNTR_MODE_STOP, 0);
  
  /* diagnostic mode runs the instruction in the tranceivers */
  PutCommandReg(CNTR_MODE_DIAG, 0);

  /* make instruction to read low byte of pc */
  instr = MakeControllerGetPCInstr(0);
  WriteInstrToTransceivers(instr);

  /* run the instruction in the transceivers */
  PutCommandReg(CNTR_MODE_DIAG, 1);

  /* value appears in low byte of status register */
  old_pc = GetStatusRegister() & 0xff;

  /* make instruction to read high byte of pc */
  instr = MakeControllerGetPCInstr(1);
  WriteInstrToTransceivers(instr);

  /* run the instruction in the transceivers */
  PutCommandReg(CNTR_MODE_DIAG, 1);

  /* value appears in low byte of status register */
  old_pc += (GetStatusRegister() & 0xff) << 8;

  PutCommandReg(CNTR_MODE_STOP, 0);

  return old_pc;
}

/* 
 * Function   : WriteInstrToTransceivers
 * Purpose    : Takes a 24 byte ascii strings representing a kestrel
 *		instruction and writes it to the instructions
 *		transceivers
 * Parameters : instr - pointer to instruction string.
 * Returns    : nothing
 * Notes      : 
 */
void WriteInstrToTransceivers(char *instr)
{
  char chunk1[9];
  char chunk2[9];
  char chunk3[9];
  int j;

  for (j = 0; j < 8; j++) {
    chunk1[j] = instr[j];
    chunk2[j] = instr[j+8];
    chunk3[j] = instr[j+16];
  }
  chunk1[8] = '\0';
  chunk2[8] = '\0';
  chunk3[8] = '\0';
  write(write_to_kestrel,Put_in_Pipe(T1_ADDRESS,WRITE),PIPE_WIDTH);
  write(write_to_kestrel, chunk3, PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(T2_ADDRESS,WRITE),PIPE_WIDTH);
  write(write_to_kestrel, chunk2, PIPE_WIDTH);
  write(write_to_kestrel,Put_in_Pipe(T3_ADDRESS,WRITE),PIPE_WIDTH);
  write(write_to_kestrel, chunk1, PIPE_WIDTH);
}


/* 
 * Function   : LoadInstructions
 * Purpose    : Loads an entire kestrel program into instruction
 *		memory. 
 * Parameters : instructions -- pointer to kestrel instructions
 *		num_instr -- number of instruction to write
 *		location -- starting address of program in instruction
 *		memory
 * Returns    : 1 - success
 * Notes      : 
 */
int LoadInstructions(char **instructions, int num_instr, int location)
{
  int i;
  int cont_pc;

  PutCommandReg(CNTR_MODE_STOP, 0);

  if (do_not_load_instr == 1) {     
    return 1;
  }

  /* save old PC, and set PC to location of first instr */
  cont_pc = GetControllerPC();
  SetControllerPC(location - 1);

  PutCommandReg(CNTR_MODE_LOAD_INS, 0);
  for (i = 0; i < num_instr; i++) {
    WriteInstrToTransceivers(instructions[i]);
    /* flipping load_toggle loads the instruction and increments
     * the pc counter, so instructions are added sequentially */
    PutCommandReg(CNTR_MODE_LOAD_INS, 1);
  }
  PutCommandReg(CNTR_MODE_STOP, 0);

  /* restore old PC value */
  SetControllerPC(cont_pc);
  return (1);
}

/* 
 * Function   : First_Step, Step
 * Purpose    : Handles single stepping a kestrel program.
 * Parameters : program - currently executing program
 * Returns    : 1 - success, 0 - failure
 * Notes      : 
 */
int First_Step(UserProgram *program)
{
  if (Run_Program(program, CNTR_MODE_STOP)) {
    Step(program);
    return (1);
  } 

  return (0);
}
int Step(UserProgram *program)
{
  int pc;

  if (program->state != PROGRAM_STATE_DONE) {
    /* controller executes one instruction when state changes to SINGLE */
    PutCommandReg(CNTR_MODE_SINGLE, 0);
    PutCommandReg(CNTR_MODE_STOP, 0);
    pc = GetControllerPC();
    if (RunningKestrel(program, ACTION_STEP) != 0) {
      if (!strcmp(program->code_instrptr[pc - program->code_offset], END_OF_PROGRAM)) {
	sprintf(print_msg, "end of program reached at address %d\n", pc);
	ScreenPrint(print_msg);
	End_Program(program);
      } else {
	sprintf(print_msg, "program stepped, now at address %d\n", pc);
	ScreenPrint(print_msg);
	DisplayInstruction(program, pc);
      }
    }
    return (1);
  } else {
    sprintf(print_msg, "program %s already completed execution.\n",
	    program->code_fileName);
    ScreenPrint(print_msg);
  }
  return (0);
}

/* end of file 'interface.c' */


