/* $Id: format.h,v 1.3 1998/05/15 05:36:17 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * format.h
 */

#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "program.h"

void printFormatMenu(void);
void chooseFormat(UserProgram *program);
void SetAllFormats(UserProgram *program, char format);
void GetFormatSRAMRange(UserProgram *program, char format);
void GetFormatRegRange(UserProgram *program, char format);
void printLoadFormatMenu(void);
void loadFormat(UserProgram *program, char format);

#endif
