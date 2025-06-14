/* $Id: kdb.c,v 1.7 1999/02/24 23:48:56 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * kdb.c - main control interface for the kestrel debugger.
 */

#include "globals.h"
#include "format.h"
#include "print.h"
#include "precision.h"
#include "interface.h"
#include "breakpoints.h"
#include "program.h"
#include "commands.h"
#include "kdb.h"
#include "getstate.h"
#include "controller.h"

void RunFromCommandLine(UserProgram *program)
{
  RunCommand(program);
  CloseUserProgram(program);
  Kill_Kestrel(); 
}

int RunCommand(UserProgram *program)
{
  if (Run_Program(program, CNTR_MODE_RUN)) {
    if (RunningKestrel(program, ACTION_RUN)) {
      End_Program(program);
      return (1);
#if 0
    } else {
      /* added by les so you can see the output q data even when
         program does not have enough input data! */
      MoveOutputDataToFile(program);
#endif
    }
  }
  return (0);
}

void DumpState(UserProgram *program, FILE *fp, int instr) 
{
  int pe, index; 
  PE_ptr pe_ptr;

  fprintf(fp, "INSTR: %d ADDR: %d\n", instr, GetControllerPC());

  for (pe = 0; pe < NumberOfProcs; pe++) {
    fprintf(fp, "PE: %d\n", pe);
    pe_ptr = program->PE[pe];
    fprintf(fp, "LREG: ");
    for (index = 0; index < 32; index++) {
      fprintf(fp, "%d ", pe_ptr->left_reg_bank[index]);
    }
    fprintf(fp, "\nRREG: ");
    for (index = 0; index < 32; index++) {
      fprintf(fp, "%d ", pe_ptr->right_reg_bank[index]);
    }
    fprintf(fp, "\nSRAM: ");
    for (index = 0; index < 256; index++) {
      fprintf(fp, "%d ", pe_ptr->sram_bank[index]);
    }
    fprintf(fp, "\nLATCH: ");
    for (index = 0; index < 7; index++) {
      fprintf(fp, "%d ", pe_ptr->latch_array[index]);
    }
    fprintf(fp, "\n");
  }

}

void DumpLoop(UserProgram *program, int stepped_yet)
{
  FILE *fp;
  int instr;

  fp = fopen("state.dump", "w");
  if (fp == NULL) {
    fprintf(stderr, "Could not open state.dump\n");
    return;
  }

  if (!stepped_yet) {
    First_Step(program);
  }
  
  instr = 0;
  while (program->state != PROGRAM_STATE_DONE) {
    Step(program);
    Step(program);
    FetchStateFromArray(program, PRINT_TYPE_ALL, 0, 255);
    DumpState(program, fp, instr);
    instr++;
  }

  fclose(fp);
}

void RunKdb(UserProgram *program)
{
  int stepped_yet = 0;
  char *command;
  int command_selection;

  InitializeCommandLine();

  ScreenPrint("Starting the Kestrel Debugger (kdb)\n"); 
  printMenu();
  for(;;)
    { 
      CurrentMenu = 'm'; /*assignment of global variable*/ 
      if (lastMenu != CurrentMenu) {
	printf("@Main Menu:\n");
      }
      if ((command = GetInput("kdb> ", 1)) == NULL) {
	continue;
      }
      lastMenu = CurrentMenu;

      command_selection = InterpretInput(command);
      switch( command_selection )
	{
	case MAIN_CMD_BREAKPOINT:
	  breakpoints(program);
	  break;
	case MAIN_CMD_CONTROLLER:
	  controller(program);
	  break;
	case MAIN_CMD_EXAMINE:
	  examinePE(program);
	  break;
	case MAIN_CMD_FORMAT:
	  chooseFormat(program);
	  break;
	case MAIN_CMD_HISTORY:
	  displayCommandHistory();
	  break;
	case MAIN_CMD_LIST:
	  break;
	case MAIN_CMD_MENU:
	  printMenu();
	  break;	  
	case MAIN_CMD_PRECISION:
	  choosePrecision(program);
	  break;
	case MAIN_CMD_QUIT:
	  CloseUserProgram(program);
	  QuitKdb();
	  break;
	case MAIN_CMD_RANGE:
	  whatRange(program); 
	  break;
	case MAIN_CMD_RUN:
	  stepped_yet = 0;
	  RunCommand(program);
	  break;
	case MAIN_CMD_SETTINGS:
	  printFormatInformation(program);
	  break;
	case MAIN_CMD_STEP:
	  if (stepped_yet) {
	    Step(program);
	  } else {
	    if (First_Step(program)) {
	      stepped_yet = 1;
	    }
	  }
	  break;	      
	case MAIN_CMD_DUMP:
	  DumpLoop(program, stepped_yet);
	default:
	  printf( "%s is not a valid command. ", command );
	  printf( "Type 'menu' to see a menu of commands.\n" );
	  break;
	} 
    }
}

void QuitKdb(void)
{
  Kill_Kestrel();
  exit(0);
}

/* end of file 'kdb.c' */
