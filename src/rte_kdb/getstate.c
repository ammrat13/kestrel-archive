/* $Id: getstate.c,v 1.8 1999/01/09 01:07:12 leslie Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * getstate.c - routines to get kestrel's state during program execution
 */

#include "globals.h"
#include "interface.h"
#include "program.h"
#include "code.h"
#include "getstate.h"
#include "print.h"
#include "queues.h"

extern UserProgram *user_progs; /* global variable in program.c */


/* 
 * Function   : InvalidateState
 * Purpose    : Clears "state current" flags in the UserProgram structure.
 *		The "state current variables indicate that the contents
 *		of registers and latches downloaded from the array reflect
 *		the results of the most recently executed instruction.
 *		These bits should be cleared whenever a new array instruction
 *		is executed so the data is reloaded from the array.
 * Parameters : program - UserProgram structure to update/
 * Returns    : nothing
 * Notes      : 
 */
void InvalidateState(UserProgram *program)
{
  int index;
  if (program->sram_info != NULL) {
    for (index = 0; index < 256; index++) {
      program->sram_info[index]->current = 0;
    }
  }
  program->latches_current = 0;
  program->regs_current = 0;
  program->controller_current = 0;
  program->queues_flushed = 0;
}

/* 
 * Function   : FetchStateFromArray
 * Purpose    : This routine is called before PE contents
 *		are printed in print.c and ensures that the
 *		data about to be printed reflects the current
 *		contents of the array without downloading
 *		the data from the array multiple times.
 * Parameters : program - UserProgram structure of program being debugged
 *		type - one of the PRINT_TYPE_* items from print.h that
 *		      indicates what values are about to be displayed and
 *		      should updated if they are not already.
 *		start, end - for sram locations, which locations
 *			     are about to be displayed
 * Returns    : nothing
 * Notes      : downloading the entire contents of all srams is expensive,
 *		so if the user is only printing one sram location
 *		that is not current, only that location is retreived.
 *		This should probably be expanded to several.
 */
void FetchStateFromArray(UserProgram *program,
			 int type, int start, int end)
{
  int index, invalid, first;

  switch (type) {
  case PRINT_TYPE_REGISTER :
    if (!program->regs_current) {
      GetProgramState(program, GETSTATE_REGS, 0, NULL, 0);
    }
    break;
    
  case PRINT_TYPE_SRAM :
    if (end == -1) {
      if (!program->sram_info[start]->current) {
	GetProgramState(program, GETSTATE_ONESRAM, start, NULL, 0);
      }
    } else {
      invalid = 0;
      first = -1;
      for (index = start; index <= end; index++) {
	if (program->sram_info[index]->current == 0) {
	  invalid++;
	  if (first == -1) {
	    first = index;
	  }
	}
      }
      if (invalid > 1) {
	GetProgramState(program, GETSTATE_ALLSRAM, 0, NULL, 0);
      } else if (invalid == 1) {
	GetProgramState(program, GETSTATE_ONESRAM, first, NULL, 0);
      }
    }
    break;

  case PRINT_TYPE_BSLATCH :
  case PRINT_TYPE_CLATCH :
  case PRINT_TYPE_EQLATCH :
  case PRINT_TYPE_MASKLATCH :
  case PRINT_TYPE_MDRLATCH :
  case PRINT_TYPE_MINLATCH :
  case PRINT_TYPE_MULTHILATCH :
    if (!program->latches_current) {
      GetProgramState(program, GETSTATE_LATCHES, 0, NULL, 0);
    }
    break;

  case PRINT_TYPE_ALL :
    if (!program->regs_current) {
      GetProgramState(program, GETSTATE_REGS, 0, NULL, 0);
    }
    if (!program->latches_current) {
      GetProgramState(program, GETSTATE_LATCHES, 0, NULL, 0);
    }
    invalid = 0;
    first = -1;
    for (index = 0; index < 256; index++) {
      if (program->sram_info[index]->current == 0) {
	invalid++;
	if (first == -1) {
	  first = index;
	}
      }
    }
    if (invalid > 1) {
      GetProgramState(program, GETSTATE_ALLSRAM, 0, NULL, 0);
    } else if (invalid == 1) {
      GetProgramState(program, GETSTATE_ONESRAM, first, NULL, 0);
    }
    break;

  default :
    ErrorPrint("FetchStateFromArray", "unknown type.");
  }
}

/* 
 * Function   : ParseRegisterState
 * Purpose    : Parses the output of the get-register-state program
 *		that just ran on Kestrel.  It takes the output and stores
 *		it in the correct location in the current program's
 *		PE contents buffer.
 * Parameters : getstate->output_buffer - buffer containing
 *			contents of register dump from the getstate program
 *		getstate->output_last - number of elements in
 *			output buffer
 *		program->PE - destination for register contents
 * Returns    : nothing
 * Notes      : We move register contents in both directions in an attempt
 *		to read the contents of shadow registers.
 */
void ParseRegisterState(UserProgram *getstate, UserProgram *program)
{
  int reg, dir, pe, offset;

  offset = 0;
  for (reg = 0; reg < 32; reg++) {
    for (dir = 0; dir < 2; dir++) {
      for (pe = 0; pe < NumberOfProcs; pe++) {
	if (dir == 0) { /* right */
	  program->PE[(NumberOfProcs - 1) - pe]->right_reg_bank[reg] = 
	    getstate->output_buffer[offset++];
	} else { 	/* left */
	  program->PE[pe]->left_reg_bank[reg] = getstate->output_buffer[offset++];
	}
      }
    }
  }
}

/* 
 * Function   : ParseOneSramState
 * Purpose    : process the output of the get-one-sram-state program and stores
 *		the results in the current UserProgram structure for future
 *		display to the user.
 * Parameters : getstate - UserProgram structure of getstate program.
 *		program - UserProgram structure of user program being debugged
 *		sram_location - which sram location did we read
 * Returns    : boolean success/failure
 * Notes      : Format: NPROCS+1 bytes of register 0 (PE[NPROCS-1..0] for all)
 *		        NPROCS bytes MDR
 *			NPROCS bytes of sram data
 *		Reading the contents of an sram location is destructive,
 *		destroying the contents of register 0 and the mdr, so we
 *		run a program to restore that information.
 */
int ParseOneSramState(UserProgram *getstate, UserProgram *program, 
		       int sram_location)
{
  unsigned char *restore_buffer;
  int offset, index, ret;

  /* reads sram data into UserProgram structure (save result) */
  offset = (NumberOfProcs * 2) + 1;
  for (index = NumberOfProcs - 1; index >= 0; index--) {
    program->PE[index]->sram_bank[sram_location] =
      getstate->output_buffer[offset++];
  }

  /* the restore program takes the information in this order:
   * NPROCS bytes MDR, NPROCS bytes for sram location, and
   * NPROCS+1 bytes for register 0. Data flows in same direction.
   */
  restore_buffer = malloc( (NumberOfProcs * 3 + 1) );
  if (restore_buffer == NULL) {
    ErrorPrint("ParseOneSramState", "Out of memory.");
    return 0;
  }

  /* move MDR and SRAM data*/
  offset = NumberOfProcs + 1;
  for (index = 0; index < NumberOfProcs * 2; index++) {
    restore_buffer[index] = getstate->output_buffer[offset++];
  }

  /* move register data */
  offset = 0;
  for (index = 0; index < NumberOfProcs + 1; index++) {
    restore_buffer[(NumberOfProcs * 2) + index] = getstate->output_buffer[offset++];
  }

  if (!(ret = GetProgramState(program, GETSTATE_RESTOREONESRAM, sram_location,
			      restore_buffer, (NumberOfProcs * 3) + 1))) {
    ErrorPrint("ParseOneSramState", "GetProgramState failed.");
  }

  free(restore_buffer);
  return ret;
}

/* 
 * Function   : ParseAllSramFormat
 * Purpose    : process the output of the get-all-sram-state program
 *		and put the data in the current UserProgram structure for
 *		future display to the user.
 * Parameters : getstate - UserProgram structure for the getstate program
 *			just run on the array (contains results in output_buffer)
 *		program - UserProgram structure of the program being debugged
 * Returns    : boolean succes/failure
 * Notes      : data format: NPROCS + 1 bytes register 0 (PE[NPROCS-1..0] for all)
 *			     NPROCS bytes MDR, 
 *			     NPROCS bytes sram 0, ...  sram 255
 *		This routines users ParseOneSramState to restore the
 *		state of the array and process the data for the first
 *		sram location.
 */
int ParseAllSramState(UserProgram *getstate, UserProgram *program)
{
  int index, offset, pe;

  if (!ParseOneSramState(getstate, program, 0)) {
    ErrorPrint("ParseAllSramState", "ParseOneSramState failed.");
    return (0);
  }

  /* ParseOneSramState took care of sram location zero */
  offset = (NumberOfProcs * 3) + 1;	/* skip other data */
  for (index = 1; index < 256; index++) {
    for (pe = NumberOfProcs - 1; pe >= 0; pe--) {
      program->PE[pe]->sram_bank[index] = getstate->output_buffer[offset++];
    }
  }

  return (1);
}

/* 
 * Function   : ParseLatchState
 * Purpose    : process the output of the get-latch-state program and store
 *		the results in the current UserProgram structure for future
 *		display to the user.
 * Parameters : getstate - UserProgram structure of the getstate program
 *		program - UserProgram structure of the current program
 * Returns    : boolean success/failure
 * Notes      : data format: NPROCS + 1 bytes register 0 (PE[NPROCS-1..0] for all)
 *			     NPROCS bytes MDR,
 *			     NPROCS bytes multhi,
 *			     NPROCS bytes bitshifter,
 *			     NPROCS bytes cLatch (boolean 0/255),
 *			     NPROCS bytes eqLatch (boolean 0/255),
 *			     NPROCS bytes minLatch (boolean 0/255),
 *			     NPROCS bytes maskLatch (boolean 0/255),
 *		Reading the latches destroys the contents of register 0,
 *		so we run a restore program with the appropriate data input.
 */
int ParseLatchState(UserProgram *getstate, UserProgram *program)
{
  int type, pe, offset, ret;

  offset = NumberOfProcs + 1;
  for (type = 0 ; type < 7; type++) {
    for (pe = NumberOfProcs - 1; pe >= 0; pe--) {
      switch(type) {
      case 0 :
	program->PE[pe]->latch_array[LATCH_ARRAY_MDRLATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 1:
	program->PE[pe]->latch_array[LATCH_ARRAY_MULTHILATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 2:
	program->PE[pe]->latch_array[LATCH_ARRAY_BSLATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 3:
	program->PE[pe]->latch_array[LATCH_ARRAY_CLATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 4:
	program->PE[pe]->latch_array[LATCH_ARRAY_EQLATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 5:
	program->PE[pe]->latch_array[LATCH_ARRAY_MINLATCH] =
	  getstate->output_buffer[offset++];
	break;
      case 6:
	program->PE[pe]->latch_array[LATCH_ARRAY_MASKLATCH] =
	  getstate->output_buffer[offset++];
	break;
      }
    }
  }

  if (!(ret = GetProgramState(program, GETSTATE_RESTOREREGZERO, 0,
			      getstate->output_buffer, NumberOfProcs + 1))) {
    ErrorPrint("ParseLatchState", "GetProgramState failed.");
  }

  return ret;
}


/* 
 * Function   : GetProgramState
 * Purpose    : Workhorse function that runs programs on the
 *		kestrel board to retrieve the state of registers, latches,
 *		and sram locations.  Much of the code here resembles that
 *		in the End_Program and Run_Program routines for regular
 *		user programs.  Since many types of getstate operations are
 *		destructive, this routine can also run programs to restore
 *		the state of items changed by the getstate programs.
 * Parameters : program - UserProgram structure of current program where
 *		 	  array memory contents are stored
 *		type - GETSTATE_* numbers that indicate which program
 *		       should be run on the board.
 *		sram_location - for the GETSTATE_ONESRAM and 
 *			GETSTATE_RESTOREONESRAM, this indicates which sram location
 *			to operate on.
 *		input_buffer - for  GETSTATE_RESTOREONESRAM and 
 *			GETSTATE_RESTOREREGZERO, this points to a buffer
 *			of data to provide as input to the program.
 *		input_total - number of elements in input_buffer
 * Returns    : boolean success/failure
 * Notes      : 
 */
int GetProgramState(UserProgram *program, int type, 
		    int sram_location, 
		    unsigned char *input_buffer, 
		    int input_total)
{
  int num_instr, location, error, index, output_size;
  unsigned char *regdata;
  char **gs_prog, **getstate_program, *getstate_name;
  UserProgram *getstate_prg;

  /* preserve the state of program currently running */
  SaveControllerState(program);
  FlushQueues(program);

  getstate_prg = calloc(1, sizeof(UserProgram));
  if (getstate_prg == NULL) {
    ErrorPrint("GetProgramState", "Out of memory.");
    return (0);
  }

  /* get information about the program to run */
  /* the assembled object code for these programs is stored in
   * getstate_prgs.c */
  switch(type) {
  case GETSTATE_REGS :
    /* execute program in getstate_regs.kasm */
    output_size = NumberOfProcs * 32 * 2;
    getstate_program = getstate_registers;
    getstate_name = "get register state";
    break;
  case GETSTATE_ONESRAM :
    /* execute program in getstate_onesram.kasm */
    if (sram_location > 255 || sram_location < 0) {
      ErrorPrint("GetProgramState", "Bad sram_location for GETSTATE_ONESRAM.");
      return (0);
    }
    output_size = NumberOfProcs * 3 + 1;
    getstate_program = getstate_onesram;
    getstate_name = "get one sram state";
    break;
  case GETSTATE_ALLSRAM :
    /* execute program in getstate_allsram.kasm */
    output_size = NumberOfProcs * 258 + 1;
    getstate_program = getstate_allsram;
    getstate_name = "get all sram state";
    break;
  case GETSTATE_RESTOREONESRAM:
    /* execute program in restore_onesram.kasm */
    if (sram_location > 255 || sram_location < 0) {
      ErrorPrint("GetProgramState", "Bad sram_location for GETSTATE_RESTOREONESRAM.");
      return (0);
    }
    output_size = 0;
    getstate_program = getstate_restoreonesram;
    getstate_name  = "restore one sram state";
    break;
  case GETSTATE_LATCHES :
    /* execute program in getstate_latches.kasm */
    output_size = NumberOfProcs * 8 + 1;
    getstate_program = getstate_latches;
    getstate_name = "get latch state";
    break;	
  case GETSTATE_RESTOREREGZERO :
    /* execute program in restorestate_regzero.kasm */
    output_size = 0;
    getstate_program = getstate_restoreregzero;
    getstate_name = "restore reg zero";
    break;
  }

  /* storage for program data */
  getstate_prg->output_bufsize = output_size + 1;
  regdata = malloc(output_size + 1);
  if (regdata == NULL) {
    ErrorPrint("GetProgramState", "Out of memory.");
    return (0);
  }
  getstate_prg->output_last = 0;
  getstate_prg->output_bytesWritten = 0;
  getstate_prg->output_buffer = regdata;
  getstate_prg->next = user_progs;
  user_progs = getstate_prg;

  /* make a copy of the program to run */
  num_instr = 0;
  while (getstate_program[num_instr]) {
    num_instr ++;
  }
  gs_prog = calloc(num_instr + 1, sizeof (char *));
  if (gs_prog == NULL) {
    ErrorPrint("GetProgramState", "Out of memory.");
    return (0);
  }
  for (index = 0; index < num_instr; index++) {
    gs_prog[index] = my_strdup(getstate_program[index]);
  }
  gs_prog[index] = NULL;
  location = program->code_offset + program->code_instr + 2;

  /* look for end of program */
  for (index = 0; index < num_instr; index++) {
    if (!strcmp(gs_prog[index], END_OF_PROGRAM)) {
      sprintf(print_msg, "%s: end of program detected at instruction %d\n",
	      getstate_name, index + location);
	ScreenPrint(print_msg);
    }
  }

  /* make necessary modification to program */
  ModifyLoopIterations(gs_prog, NumberOfProcs, num_instr);
  switch (type) {
  case GETSTATE_ONESRAM :
  case GETSTATE_RESTOREONESRAM :
    ModifySRAMReadWriteImmediate(gs_prog, sram_location, num_instr);
    break;
  }

  /* configure program for execution */
  RelocateProgram(gs_prog, location, num_instr);
  LoadInstructions(gs_prog, num_instr, location);
  SetControllerPC(location);
  getstate_prg->code_fileName = my_strdup(getstate_name);
  getstate_prg->code_instrptr = gs_prog;
  getstate_prg->code_instr = num_instr;
  getstate_prg->code_offset = location;
  getstate_prg->state = PROGRAM_STATE_RUNNING;
  /*  getstate_prg->input_fileName = "none"; */

  /* configure input data, if appropriate */
  switch (type) {
  case GETSTATE_RESTOREONESRAM :
  case GETSTATE_RESTOREREGZERO :
    getstate_prg->input_mem_buffer = input_buffer;
    getstate_prg->input_total = input_total;
    getstate_prg->input_mem_current_index = 0;
    getstate_prg->useMemoryInput = 1;
  }

  /* run program */
  sprintf(print_msg, "running program %s at %d\n", getstate_name, location);
  ScreenPrint(print_msg);
  fflush(stdout);
  PutCommandReg(CNTR_MODE_RUN, 0);
  if (!(error = RunningKestrel(getstate_prg, ACTION_RUN))) {
    sprintf(print_msg, "program %s terminated with an error.", getstate_name);
    ErrorPrint("GetProgramState", print_msg);
  }

  FlushQueues(getstate_prg);

  if ((getstate_prg->input_gaveLastByte == 1 && getstate_prg->input_numQinBytes != 1) ||
      (getstate_prg->input_gaveLastByte == 0 && getstate_prg->input_numQinBytes != 0)) {
    sprintf(print_msg, "Wrong number of elemenets left in QIn: %d.", 
	    getstate_prg->input_numQinBytes);
    ErrorPrint("GetProgramState", print_msg);
  }

  /* restore array to previous state */
  RestoreControllerState(program);

  if (error) {
    if (getstate_prg->output_last != output_size) {
      sprintf(print_msg, "bad element count from %s (%d, should be %d).",
	      getstate_name, getstate_prg->output_last, output_size);
      ErrorPrint("GetProgramState", print_msg);
    }

    switch (type) {
    case GETSTATE_REGS :
      ParseRegisterState(getstate_prg, program);
      program->regs_current = 1;
      break;
    case GETSTATE_ONESRAM :
      ParseOneSramState(getstate_prg, program, sram_location);
      program->sram_info[sram_location]->current = 1;
      break;
    case GETSTATE_ALLSRAM :
      ParseAllSramState(getstate_prg, program);
      for (index = 0; index < 256; index++) {
	program->sram_info[index]->current = 1;
      }
      break;
    case GETSTATE_LATCHES :
      ParseLatchState(getstate_prg, program);
      program->latches_current = 1;
      break;
    }
  }
  free(regdata);
  CloseUserProgram(getstate_prg);

  return (error != 0);
}

/* 
 * Function   : SaveControllerState
 * Purpose    : Destructive read of controller state, saving the
 *		results in the user program structure.  This routine
 *		destorys the state of the controller, so a call to 
 *		RestoreControllerState should follow before the program
 *		resumes execution.
 * Parameters : program - UserProgram structure where controller state
 *			  gets stored
 * Returns    : nothing
 * Notes      : empties controller pc and counter stacks
 *		TODO: read PC and counter stacks
 */
void SaveControllerState(UserProgram *program)
{
  int index, status[8];
  char bitstring[100];
  static char instr[26];

  if (program->cntr_state == NULL) {
    return;
  }
  if (program->controller_current == 1) {
    return;
  }
  program->controller_current = 1;

  PutCommandReg(CNTR_MODE_STOP, 0);
  PutCommandReg(CNTR_MODE_DIAG, 0);

  for (index = 0; index < 8; index++) {
    InitKestrelInstruction(bitstring);
    ModifyInstructionField(bitstring, index, D_OUT, D_OUTSIZE);
    WriteBinToHexInstr(bitstring, instr);
    WriteInstrToTransceivers(instr);
    PutCommandReg(CNTR_MODE_DIAG, 1);
    status[index] = GetStatusRegister();
  }

  program->cntr_state->cbs = status[0] & 0xff;
  program->cntr_state->scratch = status[1] & 0xff;
  program->cntr_state->cntrstack_el = status[2] & 0xff;
  program->cntr_state->pcstack_el = status[3] & 0xff;
  program->cntr_state->pc = (status[6] & 0xff) | ((status[7] & 0xff) << 8);

  /* TODO: read out counter and pc stack elements */

  PutCommandReg(CNTR_MODE_STOP, 0);

  FlushQueues(program);
}

/* 
 * Function   : RestoreControllerState
 * Purpose    : Restores the state of the controller based on
 *		information in the UserProgram structure.  This routine
 *		assumes it is being called after SaveControllerState,
 *		as it assumes the PC and counter stacks are already
 *		empty and ready for loading
 * Parameters : program - UserProgram structure of the current program
 * Returns    : nothing
 * Notes      : TODO: restore PC and counter stacks
 */
void RestoreControllerState(UserProgram *program)
{
  char *instr;

  if (program->cntr_state == NULL) {
    return;
  }

  PutCommandReg(CNTR_MODE_STOP, 0);

  /* Create load scratch register instruction */
  instr = MakeControllerQinToScratchInstr();
  WriteInstrToTransceivers(instr);

  /* put scratch register in input queue and load into controller */
  write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(program->cntr_state->scratch, 0), PIPE_WIDTH);
  PutCommandReg(CNTRBIT_QIN_LOAD, 0);
  PutCommandReg(CNTR_MODE_STOP, 0);

  /* run load scratch register instruction */
  PutCommandReg(CNTR_MODE_DIAG, 0);
  PutCommandReg(CNTR_MODE_DIAG, 1);
  PutCommandReg(CNTR_MODE_STOP, 0);

  /* restore program counter */
  SetControllerPC(program->cntr_state->pc);

  PutCommandReg(CNTR_MODE_STOP, 0);
}

/* end of file 'getstate.c' */

