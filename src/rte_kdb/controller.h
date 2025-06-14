/* $Id: controller.h,v 1.1 1998/07/10 05:19:01 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * controller.h
 */

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

void printControllerMenu(void);
void controller(UserProgram *program);
void displayDataInStatus(UserProgram *program);
void displayDataOutStatus(UserProgram *program);
void displayControllerState(UserProgram *program);

#endif
