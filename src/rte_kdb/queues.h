/* $Id: queues.h,v 1.2 1998/08/03 22:21:41 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment
 * Copyright (C) 1998  Regents of the University of California.
 *
 * queues.h
 */

#ifndef _QUEUES_H_
#define _QUEUES_H_

int ReadQOutElement(void);
int ReadQout(UserProgram *program, int type);
int LoadQin(UserProgram *program, int type);
void SetQInAlmostEmptyTrigger(int value);
void SetQOutAlmostFullTrigger(int value);
int FlushQueues(UserProgram *program);

#endif
