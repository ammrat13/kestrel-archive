/* $Id: precision.h,v 1.3 1998/05/15 05:58:00 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * precision.h
 *
 */

#ifndef _PRECISION_H
#define _PRECISION_H

#include "program.h"
#include "pe.h"

void printPrecisionMenu(void);
void ResetAllPrecision(UserProgram *program);
void DeleteMultiPrecision(UserProgram *program);
void choosePrecision(UserProgram *program);
void printSubPrecisionMenu(void);
void addPrec(UserProgram *program);
void Singlelocation(UserProgram *program);
void MPNlocation(UserProgram *program, int precision);
int *MPnumber_mapping(UserProgram *program, reg_ptr *info, 
		      int starting_reg, int PEindex, char SSR);

#endif
