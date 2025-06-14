/* $Id: print.c,v 1.20 2001/06/13 22:47:56 ericp Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * print.c - functions to display the state of the simulator and
 *	     board to the user using their specified formating.
 */

#include <math.h>
#include "globals.h"
#include "print.h"
#include "pe.h"
#include "commands.h"
#include "program.h"
#include "kdb.h"
#include "getstate.h"

void printRangeMenu(void)
{
  printf("@Main Menu > Range Menu:\n" );
  printf("\n reg <#> [- <#>]  - examine a range of registers in each SSR" );
  printf("\n sram <#> [- <#>] - examine a range of sram locations in each PE" );
  printf("\n masklatch        - view the maskLatch");
  printf("\n minlatch         - view the minLatch");
  printf("\n bslatch          - view the bsLatch" );
  printf("\n mdrlatch         - view the mdrLatch");
  printf("\n clatch           - view the cLatch");
  printf("\n multhilatch      - view the multhilatch" );
  printf("\n eqlatch          - view the eqLatch");
  printf("\n menu             - display this Menu");
  printf("\n back             - back to Main menu" );
  printf("\n quit             - Quit kdb\n\n" );
}

void whatRange(UserProgram *program)
{
  int command_selection = -1;  
  char *command;

  printRangeMenu();
  CurrentMenu = 'r';
  
  while( command_selection != RANGE_CMD_BACK ) {
    if ((command = GetInput("kdb> ", 1)) == NULL) {
      continue;
    }

    command_selection = InterpretInput(command);
    switch( command_selection ) {
    case RANGE_CMD_BSLATCH:
      FetchStateFromArray(program, PRINT_TYPE_BSLATCH, -1, -1);
      printRange(program, PRINT_TYPE_BSLATCH, 0);
      break;

    case RANGE_CMD_CLATCH:
      FetchStateFromArray(program, PRINT_TYPE_CLATCH, -1, -1);
      printRange(program, PRINT_TYPE_CLATCH, 0);
      break;

    case RANGE_CMD_EQLATCH:
      FetchStateFromArray(program, PRINT_TYPE_EQLATCH, -1, -1);
      printRange(program, PRINT_TYPE_EQLATCH, 0);
      break;

    case RANGE_CMD_MASKLATCH:
      FetchStateFromArray(program, PRINT_TYPE_MASKLATCH, -1, -1);
      printRange(program, PRINT_TYPE_MASKLATCH, 0);
      break;

    case RANGE_CMD_MDRLATCH:
      FetchStateFromArray(program, PRINT_TYPE_MDRLATCH, -1, -1);
      printRange(program, PRINT_TYPE_MDRLATCH, 0);
      break;

    case RANGE_CMD_MINLATCH:
      FetchStateFromArray(program, PRINT_TYPE_MINLATCH, -1, -1);
      printRange(program, PRINT_TYPE_MINLATCH, 0);
      break;

    case RANGE_CMD_MULTHILATCH:
      FetchStateFromArray(program, PRINT_TYPE_MULTHILATCH, -1, -1);
      printRange(program, PRINT_TYPE_MULTHILATCH, 0);
      break;

    case RANGE_CMD_QUIT:
      QuitKdb();
      break;

    case RANGE_CMD_MENU:
      printRangeMenu();
      break;
	      
    case RANGE_CMD_BACK:
      break;

    case RANGE_CMD_REG:
      printRegisterRange(program);
      break;

    case RANGE_CMD_SRAM:
      printSramRange(program);
      break;

    default:
      printf( "Invalid command. Type 'menu' for a list of commands\n");
      break;
    }
  }

  lastMenu = CurrentMenu;
}


void printRegisterRange(UserProgram *program)
{
  int range1, range2, ret, index;
  ret = ParseRegisterRange(32, &range1, &range2);
  if (ret == -1) {
    printf( "Invalid command. Type 'menu' for a list of commands\n");
  } else if (ret == 1) {
    FetchStateFromArray(program, PRINT_TYPE_REGISTER, range1, -1);
    printRange(program, PRINT_TYPE_REGISTER, range1);
  } else if (ret == 2) {
    FetchStateFromArray(program, PRINT_TYPE_REGISTER, range1, range2);
    for (index = range1; index <= range2; index ++) {
      printRange(program, PRINT_TYPE_REGISTER, index);
    }
  }
}


void printSramRange(UserProgram *program)
{
  int range1, range2, ret, index;
  ret = ParseRegisterRange(256, &range1, &range2);
  if (ret == -1) {
    printf( "Invalid command. Type 'menu' for a list of commands\n");
  } else if (ret == 1) {
    FetchStateFromArray(program, PRINT_TYPE_SRAM, range1, -1);
    printRange(program, PRINT_TYPE_SRAM, range1);
  } else if (ret == 2) {
    FetchStateFromArray(program, PRINT_TYPE_SRAM, range1, range2);
    for (index = range1; index <= range2; index ++) {
      printRange(program, PRINT_TYPE_SRAM, index);
    }
  }
}


/* 
 * Function   : examinePE
 * Purpose    : Prints thethe entire contents of a PE specified
 *              by the user.  If any multiprecision numbers have been
 *              defined, that information is also displayed
 * Parameters : program - UserProgram structure of the current program
 * Returns    : nothing
 * Notes      : TODO: download state of PE from board
 */
void examinePE(UserProgram *program)
{
  char *command;
  int select;
  int i;
  int MPtest; /*a multiprecision test int*/

  command = strtok(NULL, white_space);
  while (command == NULL) {
    sprintf(print_msg, "Enter number of PE to be examined (0-%d): ", NumberOfProcs - 1 );
    command = GetInput(print_msg, 0);
    if (!IsNumber(command)) {
      printf("Invalid number: %s\n", command);
      command = NULL;
    }
  }
  select = atoi( command );
  if( select >= NumberOfProcs )
    {
      printf( "You have specified a PE that is " ); 
      printf( "beyond the end of the array(%d).\n", select );
      return;
    }
  MPtest = 0;
  for (i = 0; i < 32; i++) {
    if (program->reg_info[i]->precision > 8) {
      MPtest = 1;
    }
  }
  for (i = 0; i < 256; i++) {
    if (program->sram_info[i]->precision > 8) {
      MPtest = 1;
    }
  }
  if (MPtest == 1) {
    PrintMPElement(program, select);
  } else {
    printElement(program, select);
  }
}

/* 
 * Function   : printElement
 * Purpose    : prints the entire state of the specified processing
 *              element.
 * Parameters : program - information about the user program
 *              pe_number - processor to display information for
 * Returns    : nothing
 * Notes      : 
 */
void printElement(UserProgram *program, int pe_number)
{
  int index, printed;

  FetchStateFromArray(program, PRINT_TYPE_ALL, -1, -1);

  printf("Contents of PE %d\n", pe_number);

  /* print all latches */
  printf( "bsLatch=" );
  printEightbit( program->PE[pe_number]->latch_array[LATCH_ARRAY_BSLATCH], 
		 program->latch_format[LATCH_FORMAT_BSLATCH] );
  printf( ", multHiLatch=" );
  printEightbit( program->PE[pe_number]->latch_array[LATCH_ARRAY_MULTHILATCH], 
		 program->latch_format[LATCH_FORMAT_MULTHILATCH] );
  printf( ", mdrLatch=" ); 
  printEightbit( program->PE[pe_number]->latch_array[LATCH_ARRAY_MDRLATCH], 
		 program->latch_format[LATCH_FORMAT_MDRLATCH] );
  printf( ", minLatch=" );
  if (program->PE[pe_number]->latch_array[LATCH_ARRAY_MINLATCH]) {
    printf("1");
  } else {
    printf("0");
  }
  printf( ",\neqLatch=" );
  if (program->PE[pe_number]->latch_array[LATCH_ARRAY_EQLATCH]) {
    printf("1");
  } else {
    printf("0");
  }
  printf( ", cLatch=" ); 
  if (program->PE[pe_number]->latch_array[LATCH_ARRAY_CLATCH]) {
    printf("1");
  } else {
    printf("0");
  }
  printf( ", maskLatch=" );
  if (program->PE[pe_number]->latch_array[LATCH_ARRAY_MASKLATCH]) {
    printf("1");
  } else {
    printf("0");
  }

  /* print contents of left registers */
  printf("\nContents of left registers:");
  for (index = 0; index < 32; index++) {
    if(index % 4 == 0 ) {
      printf( "\n" );
    }
    printf( "L[%2d]= ", index );
    printEightbit( program->PE[pe_number]->left_reg_bank[index], 
		   program->reg_info[index]->format ); 
    printf( " " );
  } 
  printf( "\n\n" );

  /* print contents of right registers */
  printf("Contents of right registers:\n");
  printed = 1;
  for(index = 0; index < 32; index++ ) {
    if(index % 4 == 0 ) {
      printf( "\n" );
    }
    printf( "R[%2d]= ", index );
    printEightbit( program->PE[pe_number]->right_reg_bank[index], 
		   program->reg_info[index]->format ); 
    printf( " " );
  } 
  printf( "\n\n" );

  /* print all sram locations */
  printf("Contents of sram: \n");
  printed = 1;
  for(index = 0; index < 256; index++ ) {
    if ( program->PE[pe_number]->sram_bank[index] ||
	 program->sram_info[index]->format != 'u') {
      printf( "RAM[%3d]= ", index );
      printEightbit( program->PE[pe_number]->sram_bank[index], 
		     program->sram_info[index]->format ); 
      printf( " " );
      printed++;
      if(printed % 4 == 0 ) {
	printf( "\n" );
      }
    }
  } 
  printf( "\n\n" );
}


/* 
 * Function   : printRange
 * Purpose    : display a single value (latch, register or sram location)
 *              from every processor on the board.  The value is displayed
 *              in the format specified by the user; multiprecision information
 *              is not taken into account by this function.
 * Parameters : program - information about the current user program
 *              print_type - type of memory location to print
 *              location - for register or sram locations, the location to print
 * Returns    : nothing
 * Notes      : 
 */
void printRange(UserProgram *program, int print_type, int location)
{
  int index;
  char continueon;

  switch(print_type) {
  case PRINT_TYPE_BSLATCH :
    printf("\n       bs");
    break;
  case PRINT_TYPE_MULTHILATCH :
    printf("\n   multhi");
    break;
  case PRINT_TYPE_MDRLATCH : 
    printf("\n      mdr");
    break;
  case PRINT_TYPE_MINLATCH :
    printf("\n      min");
    break;
  case PRINT_TYPE_EQLATCH :
    printf("\n       eq");
    break;
  case PRINT_TYPE_CLATCH :
    printf("\n        c");
    break;
  case PRINT_TYPE_MASKLATCH :
    printf("\n     mask");
    break;
  case PRINT_TYPE_REGISTER :
    printf("\n  reg %3d", location);
    break;
  case PRINT_TYPE_SRAM :
    printf("\n sram %3d", location);
    break;
  }

  if (print_type == PRINT_TYPE_REGISTER || print_type == PRINT_TYPE_SRAM) {
    printf( " values from 0 to %d are:\n", NumberOfProcs - 1);
  } else {
    printf( "Latch values from 0 to %d are:\n", NumberOfProcs - 1);
  }
  for( index = 0; index < NumberOfProcs; index++ ) {
    printf( "%3d: ", index );
    switch(print_type) {
    case PRINT_TYPE_BSLATCH :
      printEightbit( program->PE[index]->latch_array[LATCH_ARRAY_BSLATCH], 
		     program->latch_format[LATCH_FORMAT_BSLATCH] );
      break;
    case PRINT_TYPE_MULTHILATCH :
      printEightbit( program->PE[index]->latch_array[LATCH_ARRAY_MULTHILATCH], 
		     program->latch_format[LATCH_FORMAT_MULTHILATCH] );
      break;
    case PRINT_TYPE_MDRLATCH : 
      printEightbit( program->PE[index]->latch_array[LATCH_ARRAY_MDRLATCH], 
		     program->latch_format[LATCH_FORMAT_MDRLATCH] );
      break;
    case PRINT_TYPE_MINLATCH :
      if (program->PE[index]->latch_array[LATCH_ARRAY_MINLATCH]) {
	printf("1");
      } else {
	printf("0");
      }
      break;
    case PRINT_TYPE_EQLATCH :
      if (program->PE[index]->latch_array[LATCH_ARRAY_EQLATCH]) {
	printf("1");
      } else {
	printf("0");
      }
      break;
    case PRINT_TYPE_CLATCH :
      if (program->PE[index]->latch_array[LATCH_ARRAY_CLATCH]) {
	printf("1");
      } else {
	printf("0");
      }
      break;
    case PRINT_TYPE_MASKLATCH :
      if (program->PE[index]->latch_array[LATCH_ARRAY_MASKLATCH]) {
	printf("1");
      } else {
	printf("0");
      }
      break;
    case PRINT_TYPE_REGISTER :
      printEightbit(program->PE[index]->right_reg_bank[location], 
		    program->reg_info[location]->format);
      printf(", ");
      printEightbit(program->PE[index]->left_reg_bank[location], 
		    program->reg_info[location]->format);
      break;
    case PRINT_TYPE_SRAM :
      printEightbit(program->PE[index]->sram_bank[location], 
		    program->sram_info[location]->format);
      break;
    }
    printf(" ");
    if( (index + 1) % 4 == 0 && index > 0 ) {
      printf( "\n" );
    }
    if( (index % 128 == 0) && (index > 0) ) { 
      printf( "\n------ hit 'enter' to continue, 'q' to quit ------\n");
      scanf( "%c", &continueon );
      if( continueon == 'q' ) {
	printf( "\n" );
	return;
      }
    }
  }

  printf("\n");
}


void PrintMPElement(UserProgram *program, int PEindex)
{
  int index;
  int *MPnumber;

  printf("The Actual Eight bit values in PE %d\n", PEindex);
  printElement(program, PEindex);

  /* print left side */
  printf("\nMulti Precision values for Left Register Banks:\n");
  for (index = 0; index < 32; index++) {
    switch(program->reg_info[index]->precision) {
    case 16:
    case 32:
    case 64:
      MPnumber = MPnumber_mapping(program, program->reg_info, 
				  index, PEindex, 'l');
      printMultibit(program, PRINT_REG, MPnumber, index);
      printf("\n");
      break;
    default:
      break;
    }
  }
  printf("\n");

  /* now the right side */
  printf("\nMulti Precision values for Right Register Banks:\n");
  for (index = 0; index < 32; index++) {
    switch(program->reg_info[index]->precision) {
    case 16:
    case 32:
    case 64:
      MPnumber = MPnumber_mapping(program, program->reg_info, 
				  index, PEindex, 'r');
      printMultibit(program, PRINT_REG, MPnumber, index);
      printf("\n");
      break;
    default:
      break;
    }
  }

  /* print all values from srams */
  printf("\nMulti Precision values for SRAM Banks:\n");
  for (index = 0; index < 256; index++) {
    switch(program->sram_info[index]->precision) {
    case 16:
    case 32:
    case 64:
      MPnumber = MPnumber_mapping(program, program->sram_info, 
				  index, PEindex, 's');
      printMultibit(program, PRINT_SRAM, MPnumber, index);
      printf("\n");
      break;
    default:
      break;
    }
  }
}


/* 
 * Function   : printFormatInformation
 * Purpose    : This function prints out the print format information
 *              for latches, register banks, and sram location.  The function
 *              prints the format of every register, and lists all multi-
 *              precision numbers configured in the registers and 
 *              sram banks.
 * Parameters : program - UserProgram structure for the current program.
 *                        Contains information that describes how processor
 *                        state is printed
 * Returns    : nothing
 * Notes      : makes no changes to UserProgram structure.
 */
void printFormatInformation(UserProgram *program)
{ 
  int index, MPindex;  /* indicies...*/
  int temp_reg;
  int subindex;
  int last_index;
  char last_format;
  MPindex = 0;

  printf(" Register Format:         Latch Format:\n");
  for (index = 0; index < 32; index++) {
    printf(" [%2d] - ", index);
    printFormat(program->reg_info[index]->format);
    if (index < 7) {
      switch (index) {
      case 0:
	printf("bsLatch     - ");
	printFormat(program->latch_format[LATCH_FORMAT_BSLATCH]);
	break;
      case 1:
	printf("multHiLatch - ");
	printFormat(program->latch_format[LATCH_FORMAT_MULTHILATCH]);
	break;
      case 2:
	printf("mdrLatch    - ");
	printFormat(program->latch_format[LATCH_FORMAT_MDRLATCH]);
	break;
      case 3:
	printf("minLatch    - ");
	printFormat('u');
	break;
      case 4:
	printf("eqLatch     - ");
	printFormat('u');
	break;
      case 5:
	printf("cLatch      - ");
	printFormat('u');
	break;
      case 6:
	printf("maskLatch   - ");
	printFormat('u');
	break;
      }/*end switch*/

    } else if (index == 8) {
      printf("Multi Precision information:");

    } else if (index > 8) {
      while (MPindex < 31) {
	if (program->reg_info[MPindex]->precision != 8) {
	  break;
	}
	MPindex++;
      }
      switch(program->reg_info[MPindex]->precision) {
      case 16:
	printf("[%2d] [%2d] = 16 bit int", 
	       MPindex, program->reg_info[MPindex]->next_reg); 
	break;
      case 32:
	temp_reg = MPindex;
	for (subindex = 0; subindex < 4; subindex++) {
	  printf("[%2d] ", temp_reg);
	  temp_reg = program->reg_info[temp_reg]->next_reg;
	}
	printf("= 32 bit int");
	break;
      case 64:
	temp_reg = MPindex;
	for (subindex = 0; subindex < 8; subindex++) {
	  printf("[%2d] ", temp_reg);
	  temp_reg = program->reg_info[temp_reg]->next_reg;
	}
	printf("= 64 bit int");
	break;
      default:
	break;
      }/*end switch*/
      if (MPindex != 31) {
	MPindex++;
      }
    }
    printf("\n");
  }

  /* display print format for sram memory locations */
  last_index = -1;
  last_format = 0;

  printf(" SRAM Format:\n");
  for (index = 0; index < 256; index++) {
    if (last_format != program->sram_info[index]->format || index == 255) {
      if (last_index == -1) {
	last_index = 0;
	last_format = program->sram_info[index]->format;
	continue;
      } else {
	if (index != 255) {
	  if (last_index + 1 == index) {
	    printf(" [    %3d] - ", last_index);
	  } else {
	    printf(" [%3d-%3d] - ", last_index, index - 1);
	  }
	  printFormat(last_format);
	} else {
	  if (last_format == program->sram_info[index]->format) {
	    if (last_index + 1 == index) {
	      printf(" [    %3d] - ", last_index);
	    } else {
	      printf(" [%3d-%3d] - ", last_index, index - 1);
	    }
	    printFormat(last_format);
	  } else {
	    if (last_index + 1 == index) {
	      printf(" [    %3d] - ", last_index);
	    } else {
	      printf(" [%3d-%3d] - ", last_index, index - 1);
	    }
	    printFormat(last_format);
	    printf("\n [    255] - ");
	    printFormat(program->sram_info[index]->format);
	  }
	}	
	last_index = index;
	last_format = program->sram_info[index]->format;
      } 
    } else {
      continue;
    }
    printf("\n");
  }	

  printf(" SRAM Multi Precision information:\n ");
  for (MPindex = 0; MPindex < 256; MPindex++) {
    if (program->sram_info[MPindex]->precision == 8) {
      continue;
    }
    switch(program->sram_info[MPindex]->precision) {
    case 16:
      printf("[%2d] [%2d] = 16 bit int", 
	     MPindex, program->sram_info[MPindex]->next_reg); 
      break;
    case 32:
      temp_reg = MPindex;
      for (subindex = 0; subindex < 4; subindex++) {
	printf("[%2d] ", temp_reg);
	temp_reg = program->sram_info[temp_reg]->next_reg;
      }
      printf("= 32 bit int");
      break;
    case 64:
      temp_reg = MPindex;
      for (subindex = 0; subindex < 8; subindex++) {
	printf("[%2d] ", temp_reg);
	temp_reg = program->sram_info[temp_reg]->next_reg;
      }
      printf("= 64 bit int");
      break;
    default:
      break;
    }/*end switch*/

  }
  printf("\n");
}


/* 
 * Function   : printFormat
 * Purpose    : Given a print format for a register value, this function
 *              displays  a text description of that format to stdout.
 * Parameters : format - character describing the display format
 * Returns    : nothing
 * Notes      : 
 */
void printFormat(char format)
{
  switch(format) {
  case 'u':
    printf("decimal unsigned  ");
    break;
  case 'h':
    printf("hex               ");
    break;
  case 's':
    printf("decimal signed    ");
    break;
  case 'b':
    printf("binary            ");
    break;
  case 'd':
    printf("DNA               ");
    break;
  case 'r':
    printf("RNA               ");
    break;
  case 'p':
    printf("Protein           ");
    break;
  }
}


/*the multibit precision maker*/
/*This function does much what the eightbit function does but with higher
 *precison numbers(16, 32 or 64 bit numbers). It takes the precision of the 
 *number to be converted then a char to indicate
 *the type of conversion to be done 'h' for hex, 'b' for binary 's'
 *for unsigned to signed and 'n' for no conversion (just the change
 *to a number greater than 8 bits) and finaly an array of 8 bit numbers
 *which it uses to find a 16, 32 or 64 bit number, the numbers in the array
 *need to be listed in ascending order that is the first a bits in index[0]
 *the second 8 bits in index [1] and so on.it then prints the converted 
 *number to the screen with no attempt at formatting. 
 **/
void printMultibit(UserProgram *program, int print_type,
		   int * data, int starting_reg)
{
  char format;
  int precision, result;
  
  if (print_type == PRINT_REG) {
    format = program->reg_info[starting_reg]->format;
    precision = program->reg_info[starting_reg]->precision;
  } else if (print_type == PRINT_SRAM) {
    format = program->sram_info[starting_reg]->format;
    precision = program->sram_info[starting_reg]->precision;
  }

  switch (format) {
  case 'h':
    result = printMPnumber(data, precision);
    printHex(result, precision);
    break;

  case 'b':
    result = printMPnumber(data, precision);
    printBinary (result, precision);
    break;

  case 'u':
    result = printMPnumber(data, precision);
    printf("%u", (unsigned int)result);
    break;

  case 's':
    if (precision == 16) {
      result = printMPnumber(data, precision);
      if (result > 32768) {
	result = result - 1;
	result = 65535 - result;
	printf("-%d",result);
      } else {
	printf("%d", result);
      }
    } else if (precision == 32) {
      result = printMPnumber(data, precision);
      if (result > 838860) {
	result = result - 1;
	result =  16777216 - result;
	printf("-%d",result);
      } else { 
	printf("%d", result);
      }
    } else if (precision == 64) {
      result = printMPnumber(data, precision);
      if (result > 3.602879e+16) { 
	result = result - 1;
	result = 7.205759e+16 - result;
	printf("-%d",result);
      } else {
	printf("%d", result);
      }
    }
    break;

  default :
    printf("Unknown format: %c\n", format);
    break;
  }
}      


/* this function takes a array of 8 bit numbers and converts them
 * to the precision indicated in the first argument
 **/
int printMPnumber(int * data, int precision)
{
   int result, temp, magnitude, i;
   i = (precision/8) - 1;

   result = 0;
   for (; i >= 0; i--) {
     magnitude = pow(256,i);
     temp = magnitude * data[i];
     result = result + temp;	
   }

   return result;
}


/* 
 * Function   : printEightbit
 * Purpose    : Displays a byte of information is the specified format.
 * Parameters : value - 8-bit value to print
 *		format - format of value to print:
 *			'h' - hexidecimal number;
 *			'd' - DNA sequence;
 *			'r' - RNA sequence;
 *			'p' - Protein;
 *			'u' - unsigned int;
 *			'b' - binary;
 *			's' - signed int
 * Returns    : nothing
 * Notes      : Does not handle multiprecision cases.
 */
void printEightbit(int value, char format)
{
  switch(format) {	
  case 'u':
    printf("%3d", value);
    break;
  case 'd': /*for DNA sequence*/
    printDNA(value);
    break;
  case 'r': /*for RNA sequences*/
    printRNA(value);
    break;
  case 'p':
    printProtein(value);
    break;
  case 'h' : /*do 8 bit hex conversion*/
    printHex(value, 8);
    break;
  case 'b' : /*do 8 bit binary conversion*/
    printBinary(value, 8);
    break;
  case 's' : /*do unsigned to signed conversion*/

    if (value > 128)/*if its negative find two's complement*/
      {
	value = value - 1;
	value = 255 - value;
	printf("-%3d",value);
      }
    else /*otherwise print it normally*/
      {
	printf("%3d", value);
      }
    break;
  default:
    printf("Invalid conversion type selected: %c (%d)\n", format, (int)format);
    break;
  }
}


/*This function takes a decimal number of the precision indicated in the
 *first argument and prints its hex equivalent to the screen
 **/
void printHex(int value, int precision)
{
  int result;
  int dividend;
  int i = precision / 4; 
  dividend = pow(16,(i-1));
  
  printf("0x");
  for (;i != 0; i--) {
    result = value / dividend;
    value = value % dividend;
    if ( result > 9 ) {
      printf("%c", result + 'A' - 10);
    } else {
      printf("%c", result + '0');
    }
    dividend = dividend / 16;
  }  
}


/*This Function takes a decimal number of the precision indicated in its
 *first argument and prints its binary equivalent to the screen
 *note that it is the same algorithim as the base-16 conversion 
 *minus that annoying possible char values to deal with 
 *it would work very well if say a base-8 conversion was desired
 **/
void printBinary(int value, int precision)
{
  int result;
  int dividend;
  int i = precision; 
  dividend = pow(2,(i-1));
  
  for (;i != 0; i--) {
    result = value / dividend;
    value = value % dividend;
    printf("%d", result);  /*print that zero or print that 1*/
    dividend = dividend / 2;
  }
}


void printDNA(int value)
{
  switch (value)
    {
    case 0:
      printf("  a");
      break;
    case 1:
      printf("  g");
      break;
    case 2:
      printf("  c");
      break;
    case 3:
      printf("  t");
      break;
    case 4:
      printf("  r");
      break;
    case 5:
      printf("  y");
      break;
    case 6:
      printf("  n");
      break;
    default:
      printf("%3d", value);
      break;
    }
}

void printRNA(int value)
{
 switch (value)
    {
    case 0:
      printf("  a");
      break;
    case 1:
      printf("  g");
      break;
    case 2:
      printf("  c");
      break;
    case 3:
      printf("  t");
      break;
    case 4:
      printf("  r");
      break;
    case 5:
      printf("  y");
      break;
    case 6:
      printf("  n");
      break;
    default:
      printf("%3d", value);
    break;
    }
}

void printProtein(int value)
{
  switch (value)
    {
	case 0:
		printf("  0");
		break;
	case 1: 
		printf("  a");
		break;
    case 2:
		printf("  c");
		break;
    case 3:
		printf("  d");
      break;
    case 4: 
		printf("  e");
		break;
    case 5: 
		printf("  f");
		break;
    case 6:  
		printf("  g");
		break;
    case 7:  
		printf("  h");
		break;
    case 8:  
		printf("  i");
		break;
    case 9:  
		printf("  k");
		break;
    case 10:  
		printf("  l");
		break;
    case 11:  
		printf("  m");
		break;
    case 12:  
		printf("  n");
		break;
    case 13:  
		printf("  p");
		break;
    case 14:  
		printf("  q");
		break;
    case 15:  
		printf("  r");
		break;
    case 16:  
		printf("  s");
		break;
    case 17:  
		printf("  t");
		break;
    case 18:  
		printf("  v");
		break;
    case 19:  
		printf("  w");
		break;
    case 20:  
		printf("  y");
		break;
    default:  
		printf("%3d", value);
		break;
    }
}

void printUsage(void)
{
  printf("\nUsage: kestel [option1|option2|...] objectfile inputfile|(-in inputfile +) ouputfile [#procesors] \n\n");
  printf("objectfile    -- source code to be executed\n");
  printf("inputfile     -- the file containing the input for this program\n");
  printf("outputfile    -- the file that the output should be written to\n");
  printf("[#processors] -- The number of processors to simulate (only valid w/ -s)\n");
  printf(" options:\n");
  printf("   -debug      -- run kdb debugger\n");
  printf("   -[s|b]      -- specified -b for board or -s for simulator\n");
  printf("   -slow       -- use only low-level functions.\n");
  printf("   -showirq    -- show interrupts from board (only works with -slow or -s)\n");
  printf("   -noloadi    -- don't load kestrel program\n");
  printf("   -iformat|-oformat [format] -- format of in/out data file\n");
  printf("      [format] is ont of the following:\n");
  printf("      decimal -- ascii decimal data (newline separated)\n");
  printf("      octal   -- ascii octal data (newline separated)\n");
  printf("      hex     -- asci hexidecimal data (newline separated)\n");
  printf("      binary  -- raw data file \n");	
  printf("   -iremote   -- input file is on the machine with the board.\n");
  printf("   -oremote   -- output file is on the machine with the board.\n");
  printf("   -machine [merlin|marsh} -- force use of a specific machine.\n");
  printf(" Multiple input files, each in different formats and/or remote is\n");
  printf(" specified using any number of the following syntax:\n");
  printf("   -iformat ... -iremote -in fileName \n");
  printf("\n");
}

void help(void)
{
  printf("There are a few commands universal throughout kdb\n q - quit kdb\n");
  printf("m - print current Menu\n c - print Current location\n ? - for" );
  printf( "help (this display)\n\n");
}

void printMenu(void)
{
  printf("\n@Main Menu:\n" );
  printf("\n run        - Run your program" );
  printf("\n step       - single Step through your program");
  printf("\n examine    - Examine a specific PE");
  printf("\n range      - examine single value across a Range of PEs");
  printf("\n controller - examine state of the controller");
  printf("\n list       - List most recent instructions");
  printf("\n breakpoint - set, change or remove Breakpoints");
  printf("\n format     - change the display format ");
  printf("\n precision  - change the Precision" );
  printf("\n settings   - print current format information");
  printf("\n history    - print history buffer of commands");
  printf("\n menu       - display this Menu");
  printf("\n quit       - Quit kdb" );
  printf("\n dump       - Single step program, dumping state to file 'state.dump'\n\n");
}

/* end of file 'print.c' */

