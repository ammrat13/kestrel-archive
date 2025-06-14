/* $Id: breakpoints.c,v 1.4 1998/11/20 16:28:17 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * breakpoints.c - routines to handle user program breakpoints 
 */

#include "globals.h"
#include "program.h"
#include "breakpoints.h"
#include "commands.h"
#include "kdb.h"
#include "code.h"

/* 
 * Function   : ClearAllBP
 * Purpose    : clears all the breakpoints currently set by the program
 * Parameters : program - UserProgram structure of currently executing program
 * Returns    : nothing
 * Notes      : 
 */
void ClearAllBP(UserProgram *program)
{
  int index;

  for (index = 0; index < program->code_instr; index++) {
    if (program->code_breakpoints[index] != 0) {
      SetInstructionBP( program->code_instrptr[index], 'd');
      program->code_breakpoints[index] = 0;
    }
  }

  program->code_nextbps = 0;

  /* TODO: download modified instructions to the board */

  printf("kestrel rte: All breakpoints cleared.\n");
}


/* 
 * Function   : CheckBPIncompatibleInstr
 * Purpose    : Ensure the instruction number specified by the
 *              user is compatible with having a breakpoint set.
 * Parameters : program - UserProgram structure of the current program.
 *              break_line - instruction number to check.
 * Returns    : 1 - instruction is compatible with a breakpoint
 *              0 - instruction cannot have its break bit set 
 * Notes      : Incompatible instructions are LOOP and HALT
 *              (Are there more??)
 */
int CheckBPIncompatibleInstr(UserProgram *program, int break_line)
{
  if (break_line >= program->code_instr || break_line < 0) {
    printf("Program line %d does not exist.\n", break_line);
    return (0);
  }
 
  /* TODO: code to detect loop and halt instructions */

  return (1);
}


/* 
 * Function   : SetBP
 * Purpose    : Sets a breakpoints at a line specified by the user.
 * Parameters : program - UserProgram structure for the current program
 * Returns    : 1 - success, 0 - failure
 * Notes      : breakpoints cannot be set at all instruction types.
 */
int SetBP(UserProgram *program)
{
  char *command;
  int break_line;

  command = strtok(NULL, white_space);
  while (command  == NULL) {
      sprintf(print_msg, "\nEnter line number (0 to %d): ", program->code_instr - 1);
      command = GetInput(print_msg, 0);
  }
  
  if (!IsNumber(command)) {
    printf("Invalid line number: %s\n", command);
    return (0);
  }
  break_line = atoi(command);

  if (!CheckBPIncompatibleInstr(program, break_line)) {
    return (0);
  }

  if (SetInstructionBP( program->code_instrptr[break_line], 's')) {
    printf("Breakpoint already set at instruction %d\n", break_line);
    return (0);
  }

  program->code_breakpoints[break_line] = ++program->code_nextbps;

  printf("kestrel rte: Breakpoint #%d set at instruction %d\n", 
	 program->code_nextbps, break_line);

  /* TODO: dowmload modified instruction to board */

  return (1);
}

/* 
 * Function   : DeleteBP
 * Purpose    : Removes a breakpoint at a lines specified by the user.
 * Parameters : program - UserProgram structure of the current program.
 * Returns    : 1 - success, 0 - failure
 * Notes      : 
 */
int DeleteBP(UserProgram *program)
{
  char *command;
  int break_line;

  command = strtok(NULL, white_space);
  while(command == NULL) {
    sprintf(print_msg, "\nEnter program line (0 to %d): ", program->code_instr - 1);
    command = GetInput(print_msg, 0);
  }
    
  if (!IsNumber(command)) {
    printf("Invalid Number: %s\n", command);
    return (0);
  }
  break_line = atoi(command);
  
  if (!CheckBPIncompatibleInstr(program, break_line)) {
    return (0);
  }
  
  if (program->code_breakpoints[break_line] == 0) {
    printf("No breakpoint current set on line %d\n", break_line);
    return (0);
  }

  SetInstructionBP( program->code_instrptr[break_line], 'd');

  printf("kestrel rte: Breakpoint %d on line %d removed.\n", 
	 program->code_breakpoints[break_line], break_line);

  program->code_breakpoints[break_line] = 0;

  /* TODO: download modified instruction to the board */

  return (1);
}


/* 
 * Function   : DisplayBP
 * Purpose    : Prints a list of all the currently set breakpoints
 * Parameters : program - UserProgram structure of the current program
 * Returns    : nothing
 * Notes      : 
 */
void DisplayBP(UserProgram *program)
{
  int index, count = 0;

  for (index = 0; index < program->code_instr; index++) {
    if (program->code_breakpoints[index] != 0) {
      printf("Breakpoint #%d set at line %d.\n",
	     program->code_breakpoints[index], index);
      count++;
    }
  }
  if (count == 0) {
    printf("No breakpoints set.\n");
  } else {
    printf("\n%d breakpoints set.\n", count);
  }
}

void Print_bp_Menu(void)
{
  printf("@Main Menu > Breakpoint Menu \n");
  printf("\n set     - Set a new breakpoint");
  printf("\n clear   - Clear all breakpoints");
  printf("\n delete  - remove a specific breakpoint");
  printf("\n display - display all breakpoints currently set");
  printf("\n menu    - display this menu");
  printf("\n back    - Return to Main menu" );
  printf("\n quit    - Quit kdb\n\n" );
}

void breakpoints(UserProgram *program)
{
  int command_selection = -1;
  char *command;

  Print_bp_Menu();
  CurrentMenu = 'b';
  
  while(command_selection != BP_CMD_BACK)
  {
    if ((command = GetInput("kdb> ", 1)) == NULL) {
      continue;
    }

    command_selection = InterpretInput(command);
    switch(command_selection)
      {
      case BP_CMD_CLEAR: 
	ClearAllBP(program);
	break;
      case BP_CMD_DELETE:
	DeleteBP(program);
	break;
      case BP_CMD_DISPLAY:
	DisplayBP(program);
	break;
      case BP_CMD_MENU:
	Print_bp_Menu();
	break;
      case BP_CMD_BACK:
	break;
      case BP_CMD_QUIT:
	QuitKdb();
	break;
      case BP_CMD_SET:
	SetBP(program);
	break;
      default:
	printf("Invalid command type 'menu' to see a list of commands\n");
	break;
      }
  }/*end while*/

  lastMenu = CurrentMenu;
}


/* end of file 'breakpoints.c' */
