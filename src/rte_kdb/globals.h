/* $Id: globals.h,v 1.11 1998/11/20 16:30:57 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * globals.h - main include file for the kestrel rte.
 */

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include "config.h"

/* global variables used across multiple files */

extern char lastMenu;
extern char CurrentMenu; /* the current menu (used for command interpreting) */
#define MAX_INPUT_LENGTH 256
extern char input_buffer[MAX_INPUT_LENGTH];/*the grand almighty input buffer*/
extern char *white_space;/* used with strtok */
extern int write_to_kestrel; /* pipe handles for communication with board */
extern int read_from_kestrel;
extern int have_board;
extern int NumberOfProcs;   /* number of processors in kestrel array */
extern char print_msg[1024];
extern int do_not_load_instr;
extern int use_server_func;
extern int user_show_irq;

void ErrorPrint(char *func, char *msg);
void ScreenPrint(char *msg);
void DebugPrint(char *msg);

#endif

