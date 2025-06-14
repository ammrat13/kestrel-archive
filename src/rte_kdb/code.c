/* $Id: code.c,v 1.20 1998/10/13 03:00:27 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * code.c - functions that modify executable instructions.
 */

#include "globals.h"
#include "code.h"

int hextoint(char hex)
{
  int value;

  if (hex >= '0' && hex <= '9') {
    value = hex - '0';
  } else if (hex >= 'A' && hex <= 'F') {
    value = hex - 'A' + 10;
  } else if (hex >= 'a' && hex <= 'f') {
    value = hex - 'a' + 10;
  } else {
    sprintf(print_msg, "bad value: %d", (int)hex);
    ErrorPrint("hextoint", print_msg);
    return -1;
  }
  return value;
}

char inttohex(int value)
{
  char letter;

  if (value > 15 || value < 0) {
    sprintf(print_msg, "bad value: %d", value);
    ErrorPrint("inttohex", print_msg);
    return -1;
  }
  if (value >= 0 && value <= 9) {
    letter = '0' + value;
  } else if (value >= 10 && value <= 15) {
    letter = 'a' + value - 10;
  }
  return letter;
}

void ReverseBinaryInstruction(char *bitstring)
{
  int index;
  char temp;

  for (index = 0; index < 48; index++) {
    temp = bitstring[index];
    bitstring[index] = bitstring[95-index];
    bitstring[95-index] = temp;
  }
}

void CorrectForALUBitFlip(char *bitstring)
{
  int func, bit3, bit0;

  func = GetInstructionField(bitstring, FUNC, FUNCSIZE);

  bit3 = (func >> 3) & 1;
  bit0 = func & 1;
  func = func & 0xf6;
  func = func | (bit3) | (bit0 << 3);

  ModifyInstructionField(bitstring, func, FUNC, FUNCSIZE);
}


/* GenerateBinaryInstr, ModifyInstructionField, GetInstructionField,
 * and WriteBinToHexInstr are used to produce, modify, and examine kestrel
 * instructions for the run time environment.  The instruction are converted
 * from ascii-hexidecimal to ascii-binary to simplify operations on
 * the various fields.
 */
int GenerateBinaryInstr(char *instr, char *bitstring)
{
  int index, value, power;

  for(index = 0; index < 24; index++) {
    if ((value = hextoint(instr[index])) == -1) {
      sprintf(print_msg, "Bad Instruction: %s\n", instr);
      ErrorPrint("GenerateBinaryInstr", print_msg);
      return (0);
    }
    for (power = 0; power < 4; power++) {
      if ( (value >> power) & 0x1 ) {
	bitstring[(index * 4) + (3 - power)] = '1';
      } else {
	bitstring[(index * 4) + (3 - power)] = '0';
      }
    }
  }
  bitstring[index * 4] = 0;

  ReverseBinaryInstruction(bitstring);
  CorrectForALUBitFlip(bitstring);
  return (1);
}

void ModifyInstructionField(char *bitstring, int value, int position, int length)
{
  int index, shift;
  
  shift = 0;
  for (index = position; index < position + length; index++) {
    if ( (value >> shift) & 0x1 ) {
      bitstring[index] = '1';
    } else {
      bitstring[index] = '0';
    }
    shift++;
  }
}

int GetInstructionField(char *bitstring, int position, int length)
{
  int index, shift;
  int value;

  value = 0;
  shift = 0;
  for (index = position; index < position + length; index++) {
    if (bitstring[index] == '1') {
      value += (1 << shift);
    }
    shift++;
  }

  return value;
}

/* contents of bitstring are distroyed by this function */
void WriteBinToHexInstr(char *bitstring, char *dest)
{
  int index, value;

  CorrectForALUBitFlip(bitstring);
  ReverseBinaryInstruction(bitstring);

  for (index = 0; index < 96; index += 4) {
    value = ((bitstring[index    ] - '0') << 3) +
            ((bitstring[index + 1] - '0') << 2) +
            ((bitstring[index + 2] - '0') << 1) +
             (bitstring[index + 3] - '0');
    dest[index / 4] = inttohex(value);
  } 
  dest[index / 4] = 0;
}

/* 
 * Function   : InitKestrelInstruction
 * Purpose    : Creates a nop instruction that preserves the state of
 *		both the controller and the kestrel array.  This instruction
 *		serves as a starting point for producing instructions
 *		to perform controller operations needed by the run time
 *		environment.
 * Parameters : bitstring - destination memory for 96-bit binary instruction
 * Returns    : nothing
 * Notes      : nop in kestrel array: FB not a comparator op. and FUNC=mova
 * nop in controller: keep old PC.  */
void InitKestrelInstruction(char *bitstring)
{
  int index;
  
  for (index = 0; index < 96; index++) {
    bitstring[index] = '0';
  }

  /* Make the default instruction a nop instruction */

  /* makes this a "mova" instruction (opa and dest are same = L0) */
  ModifyInstructionField(bitstring, 5, FUNC, FUNCSIZE);

  /* the comparator minLatch and eqLatch can get changed if fb < 3 ||
   * (fb > 3 && fb < 7 && eqLatch), so set FB to something harmless:
   * 8=>nor from BitShifter MUX (same as assembler) */
  /* rd==wr==0 => no sram operation.
   * lc==0 => don't latch cLatch (ALU carry)
   * mp==ci==0 => no multiplication (don't latch multHi)
   * bit==0 => no bitshifter operation (keep old bitshifter contents) 
   */
  ModifyInstructionField(bitstring, 8, FB, FBSIZE);

  /* select PC from old PC (keep same) */
  ModifyInstructionField(bitstring, 3, PC_OUT_SEL, PC_OUT_SELSIZE);
}

/* 
 * Function   : MakeControllerQinToQoutInstr
 * Purpose    : Makes a controller instruction that moves a byte in
 *		QIn to QOut through the scratch register.
 * Parameters : nothing
 * Returns    : pointer to the new instruction (static data => non-reentrant)
 * Notes      : 
 */
char *MakeControllerQinToQoutInstr(void)
{
  char bitstring[100];
  static char instr[26];

  InitKestrelInstruction(bitstring);
  
  /* select scratch register from Qin */
  ModifyInstructionField(bitstring, 2, SCR_SEL, SCR_SELSIZE);
  
  /* enable scratch register latching */
  ModifyInstructionField(bitstring, 1, SCR_STORE, SCR_STORESIZE);

  /* force write to Qout from scratch register */
  ModifyInstructionField(bitstring, 1, FIFO_OUT, FIFO_OUTSIZE);

  WriteBinToHexInstr(bitstring, instr);

  return instr;
}

/* 
 * Function   : MakeControllerQinToScratchInstr
 * Purpose    : Make an instruction to move data from QIn to
 *		the scratch register,
 * Parameters : nothing
 * Returns    : static pointer to new instruction.
 * Notes      : 
 */
char *MakeControllerQinToScratchInstr(void)
{
  char bitstring[100];
  static char instr[26];

  InitKestrelInstruction(bitstring);

  /* select scratch register from Qin */
  ModifyInstructionField(bitstring, 2, SCR_SEL, SCR_SELSIZE);
  
  /* enable scratch register latching */
  ModifyInstructionField(bitstring, 1, SCR_STORE, SCR_STORESIZE);

  WriteBinToHexInstr(bitstring, instr);

  return instr;
}

/* 
 * Function   : MakeControllerGetPCInstr
 * Purpose    : Make a controller instruction to read either the
 *		low or high byte of the current PC.  This instruction
 *		will only work when the controller is in
 *		diagnostic mode.
 * Parameters : low_high - 0=>read low byte, 1=>read high byte
 * Returns    : static pointer to new instruction
 * Notes      : 
 */
char *MakeControllerGetPCInstr(int low_high)
{
  char bitstring[100];
  static char instr[26];

  InitKestrelInstruction(bitstring);

  if (low_high == 0) {
    /* read low byte of PC */
    ModifyInstructionField(bitstring, 6, D_OUT, D_OUTSIZE);
  } else {
    /* read high byte of PC */
    ModifyInstructionField(bitstring, 7, D_OUT, D_OUTSIZE);
  }
  WriteBinToHexInstr(bitstring, instr);

  return instr;
}

/* 
 * Function   : MakeControllerSetPCInstr
 * Purpose    : Make a controller instruction to set the
 *		PC to the specified value.
 * Parameters : imm - new 16 bit value for the PC
 * Returns    : static pointer to the new instruction
 * Notes      : 
 */
char *MakeControllerSetPCInstr(int imm)
{
  char bitstring[100];
  static char instr[26];

  InitKestrelInstruction(bitstring);

  /* select PC from immediate  */
  ModifyInstructionField(bitstring, 1, PC_OUT_SEL, PC_OUT_SELSIZE);

  /* set immediate value */
  ModifyInstructionField(bitstring, imm, CONT_IMM, CONT_IMMSIZE);

  WriteBinToHexInstr(bitstring, instr);
  
  return instr;
}


/* 
 * Function   : ModifySRAMReadWriteImmediate
 * Purpose    : Modify the kestrel immediate field of instructions that
 *		performs sram read and writes.  
 * Parameters : instructions - object code of program to modify
 *		location - new value for immediate field 
 *		num_instr - number of instructions of programs.
 * Returns    : nothing
 * Notes      : This routine is used to modify the immediate
 *		field of the get-one-sram-state program to specify
 *		which location should be read.
 */
void ModifySRAMReadWriteImmediate(char **instructions, int location, int num_instr)
{
  int index;
  char bitstring[100];

  for (index = 0; index < num_instr; index++) {
    GenerateBinaryInstr(instructions[index], bitstring);
    if (GetInstructionField(bitstring, WR, WRSIZE) ||
	GetInstructionField(bitstring, RD, RDSIZE)) {
      ModifyInstructionField(bitstring, location, IMM, IMMSIZE);
      WriteBinToHexInstr(bitstring, instructions[index]);
    }
  }
}

/* 
 * Function   : ModifyLoopIterations
 * Purpose    : This function modifies the controller immediate field of 
 *		every instruction that pushes a value onto the
 *		counter stack.  This has the effect of altering the
 *		number of times loops are performed in the user program.
 * Parameters : instruction - hexidecimal-encoded kestrel instructions.
 *		new_it - number of loop iterations (as given to beginLoop)
 *		num_instr - number of instruction in program.
 * Returns    : nothing.
 * Notes      : This routine is used to modify the getstate program that
 *		gets the state of the kestrel array from the board. Changing
 *		the number of times loops are performed allows for different
 *		processor counts. This is particularly useful when using the 
 *		simulator on variable size arrays.
 *		Decrements new_it before writing to instruction.
 */
void ModifyLoopIterations(char **instructions, int new_it, int num_instr)
{
  int index;
  char bitstring[100];

  for (index = 0; index < num_instr; index++) {
    GenerateBinaryInstr(instructions[index], bitstring);
    if (!GetInstructionField(bitstring, PUSH_CNT, PUSH_CNTSIZE)) {
      continue;
    }
    ModifyInstructionField(bitstring, new_it - 1, CONT_IMM, CONT_IMMSIZE);
    WriteBinToHexInstr(bitstring, instructions[index]);
  }
}


/* 
 * Function   : RelocateProgram
 * Purpose    : Kestrel programs have absolute branch targets, and
 *		those targets assume instructions are numbered starting
 *		with zero at the first instruction.  If a program is
 *		loaded into instruction memory at a location other than
 *		0, the offset of the first instruction must be added to
 *		all branch targets so they go to the correct instruction.
 *		Branch targets are stored in the immediate field of
 *		instructions with BR_0 or BR_W_OR set, or when PC_OUT_SEL
 *		selects the immediate field.
 * Parameters : instruction - hexidecimal-encoded instructions
 *		location - location in instruction memory of the first instruction
 *		num_instr - number of instructions in the program
 * Returns    : nothing
 * Notes      : 
 */
void RelocateProgram(char **instructions, int location, int num_instr)
{
  int index, value;
  char bitstring[100];

  for (index = 0; index < num_instr; index++) {
    GenerateBinaryInstr(instructions[index], bitstring);

    if (GetInstructionField(bitstring, BR_0, BR_0SIZE) ||
	GetInstructionField(bitstring, BR_W_OR, BR_W_ORSIZE) ||
	GetInstructionField(bitstring, PC_OUT_SEL, PC_OUT_SELSIZE) == 1) {
      value = GetInstructionField(bitstring, CONT_IMM, CONT_IMMSIZE);
      value += location;
      ModifyInstructionField(bitstring, value, CONT_IMM, CONT_IMMSIZE);
      WriteBinToHexInstr(bitstring, instructions[index]);
    }
  }
}

/* 
 * Function   : SetInstructionBP
 * Purpose    : Sets or clears the breakpoint bit 
 *              of the specified instruction.  When this bit is set,
 *              the controller will stop execution of the current
 *              program before this instruction is executed.
 * Parameters : obj_code - pointer to the ascii code representing the
 *                         instruction from the UserProgram structure.
 *              sord - 's' to set the breakpoint; 'd' to clear the bp;
 *		       'q' query the state of the breakpoint bit
 * Returns    : previous value of the breakpoint bit (0 or 1), or
 *              -1 if an error occurred 
 * Notes      : 
 */
int SetInstructionBP(char *obj_code, char sord)
{
  int previous_value;
  char bitstring[100];

  GenerateBinaryInstr(obj_code, bitstring);
  previous_value = GetInstructionField(bitstring, BREAK, BREAKSIZE);

  if (sord == 's') {
    ModifyInstructionField(bitstring, 1, BREAK, BREAKSIZE);
  } else if (sord == 'd') {
    ModifyInstructionField(bitstring, 0, BREAK, BREAKSIZE);
  } else if (sord != 'q') {
    /* 'q' means: do nothing, just returm the current state (always done) */
    sprintf(print_msg, "bad value for sord: %c", sord);
    ErrorPrint("SetInstructionBP", print_msg);
    return (-1);
  }

  WriteBinToHexInstr(bitstring, obj_code);

  return previous_value;
}


/* end of file 'code.c' */
