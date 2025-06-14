/* $Id: pe.c,v 1.4 1998/05/30 17:37:00 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * pe.c
 *
 */

#include <stdlib.h>
#include "pe.h"

PE_ptr PEInfoInit(void)
{
  int index;
  PE_ptr tempstruct;

  tempstruct = (PE_ptr)malloc( sizeof( processing_element) );
  
  for( index=0; index<= 31; index++ )
    {
      tempstruct->left_reg_bank[index] = 0;
      tempstruct->right_reg_bank[index] = 0;
    }
  for( index=0; index<=6; index++ )
    {
      tempstruct->latch_array[index] = 0;
    }
  for (index = 0; index < 256; index++) {
    tempstruct->sram_bank[index] = 0;
  }
  return tempstruct;
}

reg_ptr RegInfoInit(void)
{
  reg_ptr temp_ptr;

  temp_ptr = (reg_ptr)malloc( sizeof( register_info ) );

  temp_ptr->current = 0;
  temp_ptr->precision = 8;
  temp_ptr->format = 'u';
  temp_ptr->next_reg = -1;

  return temp_ptr;
}

/* end of file 'pe.c' */
