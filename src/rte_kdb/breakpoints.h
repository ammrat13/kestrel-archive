/* $Id: breakpoints.h,v 1.2 1998/05/30 17:29:30 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * breakpoints.h
 *
 */

#ifndef _BREAKPOINTS_H_
#define _BREAKPOINTS_H_


void ClearAllBP(UserProgram *program);

int CheckBPIncompatibleInstr(UserProgram *program, int break_line);

int SetBP(UserProgram *program);

int DeleteBP(UserProgram *program);

void DisplayBP(UserProgram *program);

void Print_bp_Menu(void);

void breakpoints(UserProgram *program);

#endif
