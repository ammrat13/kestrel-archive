/* $Id: commands.h,v 1.4 1998/11/20 17:38:23 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * commands.h - definitions and functions for processing
 *		user commands by the debugger kdb.
 */

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

/* command array */

#define MAIN_CMD_BREAKPOINT 0
#define MAIN_CMD_CONTROLLER 1
#define MAIN_CMD_EXAMINE 2
#define MAIN_CMD_FORMAT 3
#define MAIN_CMD_HISTORY 4
#define MAIN_CMD_LIST 5
#define MAIN_CMD_MENU 6
#define MAIN_CMD_PRECISION 7
#define MAIN_CMD_QUIT 8
#define MAIN_CMD_RANGE 9
#define MAIN_CMD_RUN 10
#define MAIN_CMD_SETTINGS 11
#define MAIN_CMD_STEP 12
#define MAIN_CMD_DUMP 13
extern char *main_commands[]; /*the commands from the main menu*/

#define CNTR_CMD_DATAIN 0
#define CNTR_CMD_DATAOUT 1
#define CNTR_CMD_QINPAD 2
#define CNTR_CMD_STATE 3
#define CNTR_CMD_MENU 4
#define CNTR_CMD_BACK 5
#define CNTR_CMD_QUIT 6
extern char *controller_commands[];

#define BP_CMD_CLEAR 0
#define BP_CMD_DELETE 1
#define BP_CMD_DISPLAY 2
#define BP_CMD_MENU 3
#define BP_CMD_BACK 4
#define BP_CMD_QUIT 5
#define BP_CMD_SET 6
extern char *bp_commands[];/* the breakpoint menu commands*/

#define FORMAT_CMD_BINARY 0
#define FORMAT_CMD_DNA 1
#define FORMAT_CMD_HEX 2
#define FORMAT_CMD_MENU 3
#define FORMAT_CMD_PROTEIN 4
#define FORMAT_CMD_QUIT 5
#define FORMAT_CMD_BACK 6
#define FORMAT_CMD_RNA 7
#define FORMAT_CMD_SIGNED 8
#define FORMAT_CMD_UNSIGNED 9
extern char *format_commands[];/* the format menu commands*/

#define LOADFMT_CMD_REG 0
#define LOADFMT_CMD_SRAM 1
#define LOADFMT_CMD_BSLATCH 2
#define LOADFMT_CMD_MDRLATCH 3
#define LOADFMT_CMD_MULTHILATCH 4
#define LOADFMT_CMD_ALL 5
#define LOADFMT_CMD_MENU 6
#define LOADFMT_CMD_BACK 7
#define LOADFMT_CMD_QUIT 8
extern char *load_format_commands[];

#define PRECISION_CMD_ADD 0
#define PRECISION_CMD_BACK 1
#define PRECISION_CMD_CLEAR 2
#define PRECISION_CMD_DELETE 3
#define PRECISION_CMD_MENU 4
#define PRECISION_CMD_QUIT 5
extern char *precision_commands[]; /*the precision menu commands*/

#define SUBPREC_CMD_16 0
#define SUBPREC_CMD_32 1
#define SUBPREC_CMD_64 2
#define SUBPREC_CMD_8  3
#define SUBPREC_CMD_MENU 4
#define SUBPREC_CMD_QUIT 5
#define SUBPREC_CMD_BACK 6
extern char *sub_precision_commands[];/*the commands for actually changing prec.*/

#define RANGE_CMD_BSLATCH 0
#define RANGE_CMD_CLATCH 1
#define RANGE_CMD_EQLATCH 2
#define RANGE_CMD_MASKLATCH 3
#define RANGE_CMD_MDRLATCH 4
#define RANGE_CMD_MENU 5
#define RANGE_CMD_MINLATCH 6
#define RANGE_CMD_MULTHILATCH 7
#define RANGE_CMD_QUIT 8
#define RANGE_CMD_BACK 9
#define RANGE_CMD_REG 10
#define RANGE_CMD_SRAM 11
extern char *range_commands[]; /*the value menu commands */

/* function prototypes */
void InitializeCommandLine(void);
int InterpretInput(char *input);
int IsNumber(char *string);
char *GetInput(char *prompt, int use_history);
int ParseRegisterRange(int limit, int *range1, int *range2);
void displayCommandHistory(void);

#endif



