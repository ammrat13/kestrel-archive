/* $Id: commands.c,v 1.4 1998/11/20 17:38:22 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * commands.c - structures and routines to process user commands.
 */

#include "globals.h"
#include "commands.h"
#include "kdb.h"

#if HAVE_LIBREADLINE
#if HAVE_READLINE_H
#include "readline.h"
#else
char *readline(char *);
int history_expand(char *line, char **expansion);
void add_history(char *);
void using_history(void);
extern int history_base;
typedef struct _hist_entry {
  char *line;
  char *data;
} HIST_ENTRY;
HIST_ENTRY **history_list(void);
#endif
#endif

char *main_commands[] = {
  "breakpoint",
  "controller",
  "examine",
  "format",
  "history",
  "list",
  "menu",
  "precision",
  "quit",
  "range",
  "run",
  "settings",
  "step",
  "dump",
  NULL
};

char *controller_commands[] = {
  "datain",
  "dataout",
  "padqin",
  "state",
  "menu",
  "back",
  "quit",
  NULL
};

char *bp_commands[] = {
  "clear",
  "delete",
  "display",
  "menu",
  "back",
  "quit",
  "set",
  NULL
};

char *format_commands[] = {
  "binary",
  "dna",
  "hex",
  "menu",
  "protein",
  "quit",
  "back",
  "rna",
  "signed",
  "unsigned",
  NULL
};
  
char *load_format_commands[] = {
  "reg",
  "sram",
  "bslatch",
  "mdrlatch",
  "multhilatch",
  "all",
  "menu",
  "back",
  "quit",
  NULL
};

char *precision_commands[] = {
  "add",
  "back",
  "clear",
  "delete",
  "menu",
  "quit",
  NULL
};

char *sub_precision_commands[] = {
  "16",
  "32",
  "64",
  "8",
  "menu",
  "quit",
  "back",
  NULL
};

char *range_commands[] = {
  "bslatch",
  "clatch",
  "eqlatch",
  "masklatch",
  "mdrlatch",
  "menu",
  "minlatch",
  "multhilatch",
  "quit",
  "back",
  "reg",
  "sram",
  NULL
};

int InterpretInput(char *input)
{
  int local_command_index = 0;
  char **command_array;
  
  if ( !strcmp(input, "b") )/*typing 'b' from anywhere is always back*/
    {
      strcat(input, "ack");
    }
  
  if( !strcmp(input, "m"))/*so typing 'm' from anywhere is always menu*/
    {
      strcat(input, "enu");
    }

  switch( CurrentMenu )
    {
    case 'm':  /* main menu */
      command_array = main_commands;
      break;

    case 'f':  /* format menu */
      command_array = format_commands;
      break;
     
    case 'a': /*alter...sub_precision menu */
      command_array = sub_precision_commands;
      break;
      
    case 'b': /* breakpoint manu */
      command_array = bp_commands;
      break;
      
    case 'r':  /* value menu */
      command_array = range_commands;
      break;
      
    case 'p':  /* precision menu */
      command_array = precision_commands;
      break;
      
    case 'l':  /* precision menu */
      command_array = load_format_commands;
      break;
      
    case 'c': /* controller menu */
      command_array = controller_commands;
      break;

    default:
      printf("InterpretInput: ERROR: known command menu.\n");
      return -1;
    }
  
  for (local_command_index = 0; 
       command_array[local_command_index] != NULL;
       local_command_index++) {
    if (!strcmp(command_array[local_command_index], input)) {
      return local_command_index;
    }
  }
  return -1;
}

/* 
 * Function   : IsNumber
 * Purpose    : Determines whether the given string is a number
 * Parameters : string - string to test for numberhood.
 * Returns    : 1 - string is a number (use atoi), 0 - not a number
 * Notes      : 
 */
int IsNumber(char *string)
{
  int i;
  if (string == NULL) {
    return (0);
  }
  for (i = 0; string[i]; i++) {
    if (string[i] < '0' || string[i] > '9') {
      break;
    }
  }
  if (string[i]) {
    return (0);
  } else {
    return (1);
  }
}

void displayCommandHistory(void)
{
#if HAVE_LIBREADLINE
  HIST_ENTRY **the_list;
  int i;
     
  the_list = history_list ();
  if (the_list) {
    for (i = 0; the_list[i]; i++) {
      printf ("%d: %s\n", i + history_base, the_list[i]->line);
    }
  }
#else
  printf("Sorry, history not available (HAVE_READLINE_LIB not defined).\n");
#endif

}

void InitializeCommandLine(void)
{
#if HAVE_LINREADLINE
  using_history();
#endif
}

char *GetInput(char *prompt, int use_history)
{
#if HAVE_LIBREADLINE
  char *line = (char *)NULL;
  while (line == NULL) {
    line = readline(prompt);
    if (line != NULL) {
      if (use_history) {
	char *expansion;
	int result;

	result = history_expand (line, &expansion);
	if (result) {
	  fprintf (stderr, "%s\n", expansion);
	}
	if (result < 0 || result == 2) {
	  free (expansion);
	  continue;
	}
     
	add_history (expansion);
	strncpy (input_buffer, expansion, sizeof(input_buffer) - 1);
	free (expansion);
      } else {
	strncpy (input_buffer, line, sizeof(input_buffer) - 1);
      }
      input_buffer[MAX_INPUT_LENGTH - 1] = 0;
      free(line);
    }
  }
#else
  printf("%s", prompt);
  fgets(input_buffer, MAX_INPUT_LENGTH, stdin);
#endif
  return strtok(input_buffer, white_space);
}

/* 
 * Function   : ParseRegisterRange
 * Purpose    : Parse a user command specifying a register number or
 *              a range of registers specified as r1 - r2.
 * Parameters : string - first token of command line
 *              range1 (returned) - first register number entered by user
 *              range2 (returned) - second register (check return value)
 * Returns    : -1 - first argument was not a number (unknown command)
 *              0 - no valid register range was found
 *              1 - a single register number was found and returned
 *                  in range1 (range2 is left undefined).
 *              2 - a range of registers was found. range1 is
 *                  guaranteed to be strictly less than range2.
 *              For all return values other than -1, an appropriate 
 *              error message is printed.
 * Notes      : Assumes the caller already called strtok for the
 *              command line, so further calls will return the rest
 *              of the tokens (if any).
 */
int ParseRegisterRange(int limit, int *range1, int *range2)
{
  int temp;
  static char *my_white_space = "- \n\t\r";
  char *string, *command;

  string = strtok(NULL, my_white_space);
  while (string == NULL) {
    sprintf(print_msg, "Specify Memory Location(s): #[-#]: ");
    if ((command = GetInput(print_msg, 0)) == NULL) {
      continue;
    }
    string = strtok(command, my_white_space);
    if (!IsNumber(string)) {
      printf("Invalid number: %s\n", string);
      string = NULL;
    }
  }

  *range1 = atoi(string);
  if (*range1 < 0 || *range1 > limit - 1 ) {
    printf("Bad Memory Location: %d ; Command Ignored.\n", *range1);
    return (0);
  }
  string = strtok(NULL, my_white_space);
  if (string == NULL) {
    return (1);
  }
  if (!IsNumber(string)) {
    printf("Bad Memory Location: '%s' ; Command Ignored.\n", string);
    return (0);
  }
  *range2 = atoi(string);
  if (*range2 < 0 || *range2 > limit - 1) {
    printf("Bad Memory Location: %d ; Command Ignored.\n", *range2);
    return (0);
  }

  if (*range2 < *range1) {
    temp = *range1;
    *range1 = *range2;
    *range2 = temp;
  } else if (range1 == range2) {
    return (1);
  }

  return (2);
}


/* end of file 'commands.c' */
