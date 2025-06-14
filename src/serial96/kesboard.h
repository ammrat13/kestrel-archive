/*************************************************************************
 *
 *kesboard.h
 *
 *This file contains the functions and data structures that make up the serial
 *simulator of kestrel.
 *
 *created 9/9/97 Justin Meyer justin@cse.ucsc.edu
 *
 *************************************************************************/



#ifndef _KESBOARD_H_
#define _KESBOARD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "controller.h"


/*****************
 *Some Constant Addresses for the devices on the board are defined here
 ****************/
#define STATUS_REG_ADDRESS 0
#define QIN_ADDRESS 1
#define QOUT_ADDRESS 2
#define CMD_REG_ADDRESS 3
#define T1_ADDRESS 4
#define T2_ADDRESS 5
#define T3_ADDRESS 6
#define PROG_FIFO_ADDRESS 7
#define PROG_DELAY_LINES 8
#define ENABLE_INTERRUPTS 9
#define FLUSH_QUEUES 10
#define READ_QOUT 11
#define WRITE_QIN 12

#define Q_IN_SIZE 4096
#define Q_OUT_SIZE 4096
#define QOUT_ALMOST_FULL_DEFAULT 7 /* 7 comes from the CYC48x1 FIFO chip specs */
#define QIN_ALMOST_EMPTY_DEFAULT 7
#define PROG_SIZE 65536 


/**************************************************************************
 *96 bit kestrel instruction - The structs that arrage the 96 bits of the 
 *kestrel instruction
 **************************************************************************/

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
#define CNTR_WRITE 8
#define CNTR_WRITESIZE 2
#define CBS_LOAD 10
#define CBS_LOADSIZE 1
#define CBS_SLEFT 11
#define CBS_SLEFTSIZE 1
#define SCR_SEL 12
#define SCR_SELSIZE 2
#define SCR_STORE 14
#define SCR_STORESIZE 1
#define IN_DATA_SEL 15
#define IN_DATA_SELSIZE 1
#define UNUSED_0 16
#define UNUSED_0SIZE 1
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
#define BOARD_IMM_MUX 41
#define BOARD_IMM_MUXSIZE 1
#define SPARE 42
#define SPARESIZE 2

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

typedef struct Instruction *Instruction_ptr;
typedef struct Instruction
{ 
  /* Instruction Bits Used By Controller (41 bits) */
  unsigned int pc_out_sel : 2, 
    push_pc : 1, 
    pop_pc : 1, 
    push_cnt : 1, 
    dec_cnt : 1,
    br_0 : 1,
    br_w_or : 1, 
    cntr_write : 2, 
    cbs_load : 1, 
    cbs_sleft : 1, 
    scr_select : 2, 
    scr_store : 1, 
    in_data_sel : 1, 
    unused_0 : 1, 
    data_read : 1, 
    data_write : 1, 
    fifo_out : 1, 
    break_bit : 1, 
    cont_immediate : 16, 
    d_pop_counter : 1, 
    d_out : 3,

    board_imm_mux : 1,	/* controls MUX on the board to select immediate field */

    spare : 2,		/* two space bits */

    /* Instructiom Bits Broadcast to Kestrel Array (53 bits) */
    nop : 1,    /* does not appear in instruction (always 0 in simulator) */
    force : 1,  /* force instruction execution */
    func : 5,   /* ALU function field */
    ci : 1,     /* value for carryin of ALU (if selected) */
    mp : 1,     /* ALU carryin selection: ci OR latch carry value */
    lc : 1,     /* latch the carry out of the ALU */
    finv : 1,   /* invert flag */
    fb : 5,     /* flag selection */
    bit : 4,    /* bit shifter function field */
    rm : 2,     /* how to select result from flag */
    rd : 1,     /* read SRAM location to mdr */
    wr : 1,     /* write to SRAM location (from result bus) */
    opB : 3,    /* operandB source selection */
    opA : 6,    /* operandA register selection */
    opC : 6,    /* operandC register selection */
    dest : 6,   /* destination register selection */
    imm : 8;    /* immediate value */

} Instruction;



/***************************************************************************
 *Board Devices - The structs that simulate the various devices on the board
 ***************************************************************************/
typedef struct pcidata *pcidata_ptr;
typedef struct pcidata
{
  unsigned int address : 31, /*the first 31 bits are for addressing a device*/
    read_or_write : 1; /*the last bit determines if this is a read(0) or a
			 write(1)*/
}pcidata;

typedef struct StatusReg
{
  int diagnostic_data;
  unsigned int load_done : 1, 
    break_bit : 1, 
    i_empty : 1, 
    o_full : 1, 
    i_almost_empty : 1, 
    o_almost_full : 1, 
    pc_stack_overflo : 1, 
    pc_stack_underflo : 1, 
    cntr_stack_overflo : 1, 
    cntr_stack_underflo : 1,
    valid_next_data : 1;
}StatusReg;

/* Instruction Transceiver (working instruction) */
extern char Transceiver[25];

/* The command register is represented as a single int. */
extern int CommandReg, OldCommandReg;

/*the current instruction*/
extern Instruction instruction;

/* Circular data input/output queues */
extern unsigned char QIn[Q_IN_SIZE];
extern unsigned char QOut[Q_OUT_SIZE];

extern int QInHead;
extern int QInTail;
extern int QInBytes;
extern int QOutHead;
extern int QOutTail;
extern int QOutBytes;

/* fifo almost full/empty conditions */
extern int QOutAlmostFullLevel;
extern int WriteQOutLevelPtr;
extern int QInAlmostEmptyLevel;
extern int WriteQInLevelPtr;

#define MODIFY_LEVEL_QOUT 1
#define MODIFY_LEVEL_QIN  2
#define LEVEL_EMPTY_LSB 0
#define LEVEL_EMPTY_MSB 1
#define LEVEL_FULL_LSB 2
#define LEVEL_FULL_MSB 3
#define TOTAL_LEVELS 4

/* declaration of the status register */
extern  StatusReg Status_Register;

/* the array that is program memory */
char progmem[PROG_SIZE][25]; 

int IsQInEmpty(void);
int IsQInAlmostEmpty(void);
int IsQInFull(void);
int QInPutData(unsigned char data);
int QInGetData(unsigned char *data);
int IsQOutFull(void);
int IsQOutAlmostFull(void);
int QOutPutData(unsigned char data);
int QOutGetData(unsigned char *data);
void print_queue(char in_out);
int CheckCommandRegMode(int mode);
pcidata_ptr Process_PCI_data(int foo);
int Interpret_Inst(void);
char* Get_StatusReg(void);
void Write_T_One(char* data);
void Write_T_Two(char* data);
void Write_T_Three(char* data);
char* Read_T_One(void);
char* Read_T_Two(void);
char* Read_T_Three(void);
void Init_Board(void);
void PrintInstruction(void);
void WriteQueueLevel(int type, int *level, int *counter, int new_val);
void ModifyInstructionField(char *bitstring, int value, int position, int length);
int GetInstructionField(char *bitstring, int position, int length);
int GenerateBinaryInstr(char *instr, char *bitstring);
void CorrectForALUBitFlip(char *bitstring);
void ReverseBinaryInstruction(char *bitstring);
int hextoint(char hex);

#endif
