/* $Id: pe.h,v 1.5 1998/05/30 17:37:01 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * pe.h
 *
 */

#ifndef _PE_H_
#define _PE_H_

typedef struct processing_element
{
  unsigned char left_reg_bank[32]; /*the array of this PE's left registers*/
  unsigned char right_reg_bank[32];/*the array of this PE's right registers*/
  unsigned char sram_bank[256];    /*the aray of this PE's sram back */
  unsigned char latch_array[7];/*the array of this PE's latches
		       respectively, they are as follows: */
  /*bsIndex, multHiIndex, mdrIndex, minIndex, eqIndex, cIndex, maskIndex;*/
#define LATCH_ARRAY_BSLATCH 0
#define LATCH_ARRAY_MULTHILATCH 1
#define LATCH_ARRAY_MDRLATCH 2
#define LATCH_ARRAY_MINLATCH 3
#define LATCH_ARRAY_EQLATCH 4
#define LATCH_ARRAY_CLATCH 5
#define LATCH_ARRAY_MASKLATCH 6
}processing_element, *PE_ptr;

typedef struct register_info
{
  int current;	/* has this value been read recently */
  int precision;
  char format;
  int next_reg;
}register_info, *reg_ptr;

PE_ptr PEInfoInit(void);
reg_ptr RegInfoInit(void);

#endif






