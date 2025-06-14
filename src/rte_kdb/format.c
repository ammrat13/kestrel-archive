/* $Id: format.c,v 1.6 1998/11/20 16:28:18 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * format.c - set display formats for registers and sram memory locations
 */

#include "globals.h"
#include "program.h"
#include "format.h"
#include "commands.h"
#include "kdb.h"

/*******************************************************************
 * This function is called when the user wants to specify that the *
 * output be displayed in a particular format.                     *
 *******************************************************************/

void printFormatMenu(void)
{
  printf( "\n@Main Menu > Format Menu:\n" );
  printf("\n unsigned - change format to unsigned decimal (default)" );
  printf("\n hex      - change format to hexidecimal");
  printf("\n binary   - change format to binary");
  printf("\n signed   - change format to signed decimal" );
  printf("\n dna      - change format to a DNA sequence");
  printf("\n rna      - change format to a RNA sequence");
  printf("\n protein  - change format to a protein sequence");
  printf("\n menu     - print this menu");
  printf("\n back     - back to the main menu");
  printf("\n quit     - Quit kdb\n\n" );
}

void chooseFormat(UserProgram *program)
{
  char *command;
  int command_selection = -1;

  printFormatMenu();
  CurrentMenu = 'f';

  while( command_selection != FORMAT_CMD_BACK )
    {
      if ((command = GetInput("kdb> ", 1)) == NULL) {
	continue;
      }

      command_selection = InterpretInput(command);
      switch( command_selection )
	{
	case FORMAT_CMD_BINARY:  /* binary b */
	case FORMAT_CMD_DNA:      /* dna d */
	case FORMAT_CMD_HEX:      /* hex h */
	case FORMAT_CMD_PROTEIN:  /* protein p */
	case FORMAT_CMD_RNA:      /* rna r */
	case FORMAT_CMD_SIGNED:   /* signed  s */
	case FORMAT_CMD_UNSIGNED: /* unsigned  u */
	  loadFormat(program, command[0]);
	  printf( "@Main menu > Format menu:\n" );
	  CurrentMenu = 'f';
	  break;
	case FORMAT_CMD_MENU: /* menu */
	  printFormatMenu();
	  break;
	case FORMAT_CMD_QUIT: /* quit */
	  QuitKdb();
	  break;
	case FORMAT_CMD_BACK: /* back */
	  break;
	default:
	  printf( "Invalid command. Type 'menu' for a list of commands\n");
	  break;
	}
    }

  lastMenu = CurrentMenu;
}

void printLoadFormatMenu(void)
{
  printf( "\n@Main menu > Format menu > Change What? :\n" );
  printf( "\n reg <#> [ - <#>]  - format one or more registers.");
  printf( "\n sram <#> [ - <#>] - format one or more sram locations.");
  printf( "\n bslatch           - format bit shifter register.");
  printf( "\n mdrlatch          - format sram mdr register.");
  printf( "\n multhilatch       - format multiplier multhi register.");
  printf( "\n all               - format all registers." );
  printf( "\n menu              - Display this menu.");
  printf( "\n back              - return to the previous menu.");
  printf( "\n quit              - Quit kdb.\n\n");
}

void GetFormatRegRange(UserProgram *program, char format)
{
  int range1, range2, ret, index;
  ret = ParseRegisterRange(32, &range1, &range2);
  if (ret == -1) {
    printf("Invalid command. Type 'menu' for a list of commands\n");
  } else if (ret == 1) {
    program->reg_info[range1]->format = format;
  } else if (ret == 2) {
    for (index = range1; index <= range2; index ++) {
      program->reg_info[index]->format = format;
    }
  }
}

void GetFormatSRAMRange(UserProgram *program, char format)
{
  int range1, range2, ret, index;
  ret = ParseRegisterRange(256, &range1, &range2);
  if (ret == -1) {
    printf( "Invalid command. Type 'menu' for a list of commands\n");
  } else if (ret == 1) {
    program->sram_info[range1]->format = format;
  } else if (ret == 2) {
    for (index = range1; index <= range2; index ++) {
      program->sram_info[index]->format = format;
    }
  }
}

void SetAllFormats(UserProgram *program, char format)
{ 
  int index;
  for (index  = 0; index < 256; index++) {
    program->sram_info[index]->format = format;
  }
  for( index = 0; index < 32; index++ ) {
    program->reg_info[index]->format = format;
  }
  for (index  = 0; index < 3; index++) {
    program->latch_format[index] = format;
  }
}


void loadFormat(UserProgram *program, char format)
{
  char *command;
  int command_selection = -1;

  printLoadFormatMenu();
  CurrentMenu = 'l';

  while(command_selection != LOADFMT_CMD_BACK)
    {
      if ((command = GetInput("kdb> ", 1)) == NULL) {
	continue;
      }

      command_selection = InterpretInput(command);
      switch( command_selection ) {
	case LOADFMT_CMD_REG :
	  GetFormatRegRange(program, format);
	  break;

	case LOADFMT_CMD_SRAM :
	  GetFormatSRAMRange(program, format);
	  break;

	case LOADFMT_CMD_BSLATCH:
	  program->latch_format[0] = format;
	  break;

	case LOADFMT_CMD_MDRLATCH:
	  program->latch_format[1] = format;
	  break;

	case LOADFMT_CMD_MULTHILATCH:
	  program->latch_format[2] = format;
	  break;

	case LOADFMT_CMD_ALL:
	  SetAllFormats(program, format);
	  break;

	case LOADFMT_CMD_BACK:
	  break;

	case LOADFMT_CMD_MENU:
	  printLoadFormatMenu();
	  break;

	case LOADFMT_CMD_QUIT: /* quit */
	  QuitKdb();
	  break;
	      
	default:
	  printf( "Invalid command. Type 'menu' for a list of commands\n");
	  break;
	}
    }
}

/* end of file 'format.c' */

