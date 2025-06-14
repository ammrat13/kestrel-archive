/* $Id: code.h,v 1.9 1998/10/13 03:00:28 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * code.h
 *
 */

#ifndef _CODE_H_
#define _CODE_H_

/* meaning of 96 bit kestrel instructions (lifted from kasm96) */
/* controller bit and sizes */
#define PC_OUT_SEL 0
#define PC_OUT_SELSIZE 2
#define PUSH_PC 2
#define PUSH_PCSIZE 1
#define POP_PC 3
#define POP_PCSIZE 1
#define PUSH_CNT 4
#define PUSH_CNTSIZE 1
#define DEC_COUNT 5
#define DEC_COUNTSIZE 1
#define BR_0 6
#define BR_0SIZE 1
#define BR_W_OR 7
#define BR_W_ORSIZE 1
#define CONT_IMM_SEL 8
#define CONT_IMM_SELSIZE 2
#define CBS_LOAD 10
#define CBS_LOADSIZE 1
#define CBS_SLEFT 11
#define CBS_SLEFTSIZE 1
#define SCR_SEL 12
#define SCR_SELSIZE 2
#define SCR_STORE 14
#define SCR_STORESIZE 1
#define LEFT_DATA_SEL 15
#define LEFT_DATA_SELSIZE 1
#define RIGHT_DATA_SEL 16
#define RIGHT_DATA_SELSIZE 1
#define SCR_OUT 18
#define SCR_OUTSIZE 1
#define DATA_READ 17
#define DATA_READSIZE 1
#define DATA_WRITE 18
#define DATA_WRITESIZE 1
#define FIFO_OUT 19
#define FIFO_OUTSIZE 1
#define BREAK 20
#define BREAKSIZE 1
#define CONT_IMM 21
#define CONT_IMMSIZE 16
    
/* Diagnotisc mode controller bit location and size declaration */
#define D_POP_CNTR 37
#define D_POP_CNTRSIZE 1
#define D_OUT 38
#define D_OUTSIZE 3
    
/* Kestrel chip instruction bits location and size declaration */
#define IMM 44
#define IMMSIZE 8
#define DEST 52
#define DESTSIZE 6
#define OPC 58
#define OPCSIZE 6
#define OPA 64
#define OPASIZE 6
#define OPB 70
#define OPBSIZE 3
#define WR 73
#define WRSIZE 1
#define RD 74
#define RDSIZE 1
#define RM 75
#define RMSIZE 2
#define BIT 77
#define BITSIZE 4
#define FB 81
#define FBSIZE 5
#define FINV 86
#define FINVSIZE 1
#define LC 87
#define LCSIZE 1
#define MP 88
#define MPSIZE 1
#define CI 89
#define CISIZE 1
#define FUNC 90
#define FUNCSIZE 5
#define FORCE 95
#define FORCESIZE 1

#define END_OF_PROGRAM "b01000000000020000118003"

int hextoint(char hex);
char inttohex(int value);
void ReverseBinaryInstruction(char *bitstring);
void CorrectForALUBitFlip(char *bitstring);
int GenerateBinaryInstr(char *instr, char *bitstring);
void ModifyInstructionField(char *bitstring, int value, int position, int length);
int GetInstructionField(char *bitstring, int position, int length);
void WriteBinToHexInstr(char *bitstring, char *dest);
void ModifySRAMReadWriteImmediate(char **instructions, int location, int num_instr);
void ModifyLoopIterations(char **instructions, int new_it, int num_instr);
void RelocateProgram(char **instructions, int location, int num_instr);
int SetInstructionBP(char *obj_code, char sord);
char *MakeControllerQinToQoutInstr(void);
char *MakeControllerGetPCInstr(int low_high);
char *MakeControllerSetPCInstr(int imm);
char *MakeControllerQinToScratchInstr(void);
void InitKestrelInstruction(char *bitstring);


#endif
