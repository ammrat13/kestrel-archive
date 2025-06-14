/* $Id: kdb.h,v 1.2 1998/05/30 17:36:03 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * kdb.h - main control interface for the Kestrel debugger.
 */

#ifndef _KDB_H_
#define _KDB_H_

#include "program.h"

void RunFromCommandLine(UserProgram *program);
int RunCommand(UserProgram *program);
void RunKdb(UserProgram *program);
void QuitKdb(void);

#endif
