/* $Id: getstate.h,v 1.1 1998/05/30 17:42:56 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * getstate.h
 */

#ifndef _GETSTATE_H_
#define _GETSTATE_H_

/* type field to GetProgramState */
#define GETSTATE_REGS   	1
#define GETSTATE_ONESRAM	2
#define GETSTATE_ALLSRAM	3
#define GETSTATE_RESTOREONESRAM	4
#define GETSTATE_LATCHES	5
#define GETSTATE_RESTOREREGZERO 6

extern char *getstate_registers[];
extern char *getstate_onesram[];
extern char *getstate_allsram[];
extern char *getstate_restoreonesram[];
extern char *getstate_restoreregzero[];
extern char *getstate_latches[];

int GetProgramState(UserProgram *program, int type, 
		    int parameter, unsigned char *buffer, int buf_size);
void RestoreControllerState(UserProgram *program);
void SaveControllerState(UserProgram *program);
int ParseAllSramState(UserProgram *getstate, UserProgram *program);
int ParseOneSramState(UserProgram *getstate, UserProgram *program, int location);
void ParseRegisterState(UserProgram *getstate, UserProgram *program);
void InvalidateState(UserProgram *program);
void FetchStateFromArray(UserProgram *program,
			 int type, int start, int end);
int ParseLatchState(UserProgram *getstate, UserProgram *program);

#endif
