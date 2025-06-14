/* $Id: precision.c,v 1.5 1998/11/20 16:28:19 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * precision.c - functions to configure registers and sram memory locations
 *               as multiprecision numbers so they will be printed in
 *               a more useful way.
 */

#include "globals.h"
#include "program.h"
#include "precision.h"
#include "commands.h"
#include "kdb.h"

/***********************************************************
 * choosePrecision: this function is called when the 'p'   *
 * option is chosen from the main menu.  It presents menus *
 * and makes necessary changes to the precision variable.  *
 ***********************************************************/

void printPrecisionMenu(void)
{
  printf( "@Main menu > Precision Menu:\n" );
  printf( "\n add    - Add multiprecision numbers to formatting." );
  printf( "\n delete - Delete a single multiprecsion number.");
  printf( "\n clear  - Delete all multiprecision numbers.");
  printf( "\n menu   - Display this menu.");
  printf( "\n back   - return to the main menu.");
  printf( "\n quit   - Quit kdb.\n\n");
}

void ResetAllPrecision(UserProgram *program)
{ 
  int i;
  for (i = 0; i < 32; i++) {
    program->reg_info[i]->precision = 8;
    program->reg_info[i]->next_reg = -1;
  }
  for (i = 0; i < 256; i++) {
    program->sram_info[i]->precision = 8;
    program->sram_info[i]->next_reg = -1;
  }
  printf("All numbers are now 8 bits ints\n");
}

void DeleteMultiPrecision(UserProgram *program)
{
  int delnumber;
  char *command;

  do {
    command = strtok(NULL, white_space);
    while (command == NULL) {
      printf("Please enter the location of the lowest byte\n");
      sprintf(print_msg, "of the number you want to delete <0 - 31>: ");
      command = GetInput(print_msg, 0);
      if (!IsNumber(command) ) {
	printf("No a valid number: %s\n", command);
	command = NULL;
      }
      delnumber = atoi(command);
    } 
  } while(delnumber > 31 || delnumber < 0);
  program->reg_info[delnumber]->precision = 8;

  /* clear the settings for all registers used for a
   * multiprecision number */
  while(program->reg_info[delnumber]->next_reg != -1) {
    program->reg_info[delnumber]->next_reg = -1;
    program->reg_info[delnumber]->precision = 8;
    delnumber++;
  }
}

void choosePrecision(UserProgram *program)
{
  int command_selection = -1;
  char *command;

  printPrecisionMenu();
  CurrentMenu = 'p';

  /*while input is not back*/
  while(command_selection != PRECISION_CMD_BACK) {
    if ((command = GetInput("kdb> ", 1)) == NULL) {
      continue;
    }

    command_selection = InterpretInput(command);
    switch(command_selection)
      {
      case PRECISION_CMD_ADD:
	addPrec(program);
	printf("@Main Menu > Precision Menu:\n");		
	CurrentMenu = 'p';
	break;
      case PRECISION_CMD_BACK:
	break;
      case PRECISION_CMD_CLEAR:
	ResetAllPrecision(program);
	break;
      case PRECISION_CMD_DELETE:
	DeleteMultiPrecision(program);
	break;
      case PRECISION_CMD_MENU:
	printPrecisionMenu();
	break;
      case PRECISION_CMD_QUIT:
	QuitKdb();
	break;
      default:
	printf("That is an Invalid selection type 'menu' for a list of commands\n");
	break;
      }
  }/*end while*/
  
  lastMenu = CurrentMenu;
}

void printSubPrecisionMenu(void)
{
  printf("\n@Main Menu > Precision Menu > Alter Precision:\n");
  printf("\n 8    - 8-bit (default)");
  printf("\n 16   - 16-bit");
  printf("\n 32   - 32-bit");
  printf("\n 64   - 64-bit");
  printf("\n menu - display this menu");
  printf("\n back - back to the precision menu");
  printf("\n quit - Quit kdb\n\n" );
}

void addPrec(UserProgram *program)
{
  int command_selection = -1;
  char *command;
  
  printSubPrecisionMenu();
  CurrentMenu = 'a';

  while(command_selection != SUBPREC_CMD_BACK)
    {
      if ((command = GetInput("kdb> ", 1)) == NULL) {
	continue;
      }

      command_selection = InterpretInput(command);
      switch(command_selection)
	{
	case SUBPREC_CMD_16:
	  MPNlocation(program, 16);
	  break;
	case SUBPREC_CMD_32:
	  MPNlocation(program, 32);
	  break;
	case SUBPREC_CMD_64:
	  MPNlocation(program, 64);
	  break;
	case SUBPREC_CMD_8:
	  Singlelocation(program);
	  break;
	case SUBPREC_CMD_MENU:
	  printSubPrecisionMenu();
	  break;
	case SUBPREC_CMD_QUIT:
	  QuitKdb();
	  break;
	case SUBPREC_CMD_BACK:
	  break;
	default: 
	  printf("That is an invalid command for this menu,\n\
please type 'menu' to see a list of valid commands\n");
	  break;
	}
    }

}

void Singlelocation(UserProgram *program)
{
  int change_number;
  char *command;

  do {
    command = strtok(NULL, white_space);
    while (command == NULL) {
      command = GetInput("Enter register: ", 0);
      if (!IsNumber(command)) {
	printf("Invalid Number: %s\n", command);
      }
    }
    change_number = atoi(command);
    if (change_number > 31 || change_number < 0)
      {
	printf("please enter a number between 0 ad 31\n");
      }
  } while( change_number > 31 || change_number < 0);
  program->reg_info[change_number]->precision = 8;
  program->reg_info[change_number]->next_reg = -1;
}

/*this function queries the user about the register location of each byte of 
 *a given Multi Precision Number it takes the precision of the number as its 
 *only argument and writes to the various register structs in question - only 
 *the starting register's structs precision
 *will be altered this is to make my life easier
 */
void MPNlocation(UserProgram *program, int precision)
{
/*to keep track of the precision of the current number*/
  int i, check;
  int current_reg, temp_reg;
  int starting_reg;
  char *command;
  int register_buffer[16];

  command = strtok(NULL, white_space);
  for (i = 0; i < (precision/8); i++)
    {
      while(command == NULL) {
	sprintf(print_msg, "\nEnter register number (0-31) for byte %d: ", i);
	command = GetInput(print_msg, 0);
	if (!IsNumber(command)) {
	  printf("Invalid register number: %s\n", command);
	  command = NULL;
	}
	temp_reg = atoi(command);
	if (temp_reg > 31 || temp_reg < 0) {
	  printf("Invalid Register Number %d.\n", temp_reg);
	  command = NULL;
	}
      }
      for (check = 0; check < i; check++) {
	if (register_buffer[check] == temp_reg) {
	  printf("Register %d was already input ; Command Aborted.\n", temp_reg);
	  return;
	}
      }
      for (check = 0; check < 32; check++) {
	if (program->reg_info[check]->next_reg == temp_reg ||
	    program->reg_info[temp_reg]->next_reg != -1) {
	  printf("Register %d is already part of a multiprecision number ; \
Command Aborted.\n", temp_reg);
	  return;
	}
      }

      register_buffer[i] = temp_reg;
      command = NULL;
    }

  for (i = 0; i < (precision/8); i++) {
    if (i == 0) {
      starting_reg = register_buffer[i];
      current_reg = starting_reg;
      program->reg_info[starting_reg]->precision = precision;
    } else {
      program->reg_info[current_reg]->next_reg = register_buffer[i];
      current_reg = register_buffer[i];
      program->reg_info[current_reg]->precision = 8;
      program->reg_info[current_reg]->next_reg = -1; /* in case this is the last one */
    }
  }
}

/*this function builds a array containing the actual bytes of data that make
 *up a given multi precision number and returns that array it also prints
 *out the register locations of each byte from lowest order to highest order 
 *- I hope this printing is not a problem it shouldn't
 *be as MPnumber_mapping() is only called right before multbit() -ed
 */
int *MPnumber_mapping(UserProgram *program, reg_ptr *info, 
		      int starting_reg, int PEindex, char SSR)
{
  int i; /*your standard indices*/
  int precision; /*gotta know the precision*/
  int *MPNarray;
  int current_reg;

  precision = info[starting_reg]->precision;
  MPNarray = (int*)calloc((precision/8), sizeof(int));
  if (SSR == 'l') {
    MPNarray[0] = program->PE[PEindex]->left_reg_bank[starting_reg];
    printf("L[%2d] ", starting_reg);
  } else if (SSR == 'r') {
    MPNarray[0] = program->PE[PEindex]->right_reg_bank[starting_reg];
    printf("R[%2d] ", starting_reg);
  } else if (SSR == 's') {
    MPNarray[0] = program->PE[PEindex]->sram_bank[starting_reg];
    printf("SRAM[%2d] ", starting_reg);
  } 
  current_reg = starting_reg;
  for( i = 1; i < (precision/8); i++) {
    /* first increament to the next register */
    current_reg = info[current_reg]->next_reg;
    if(SSR == 'l') {
      MPNarray[i] = program->PE[PEindex]->left_reg_bank[current_reg];
      printf("L[%2d] ", current_reg);
    } else if (SSR == 'r') {
      MPNarray[i] = program->PE[PEindex]->right_reg_bank[current_reg];
      printf("R[%2d] ", current_reg);
    } else if (SSR == 's') {
      MPNarray[i] = program->PE[PEindex]->sram_bank[current_reg];
      printf("SRAM[%2d] ", current_reg);
    }
  }
  printf("= "); 
  return MPNarray;
}


/* end of file 'precision.c' */
