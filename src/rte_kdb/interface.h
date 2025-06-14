/* $Id: interface.h,v 1.15 1998/11/20 17:37:26 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment
 * Copyright (C) 1998  Regents of the University of California.
 *
 * interface.h - functions and constants necessary for communication
 *		 with the kestrel hardware.
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include "program.h"

/*Interupts*/
#define IRQ "FFFFFFFF"
#define SYNC_SIGNAL "EEEEEEEE"
#define TERMINATE_SIGNAL "DDDDDDDD"
#define UPDATE_SIGNAL "CCCCCCCC"
#define ERROR_SIGNAL "AAAAAAAA"

/*Misc constants
**/
#define PIPE_WIDTH 9
#define WRITE 0x80000000
#define READ  0x00000000
#define HALT "000000000000000000000000"

/* These defines are the major control bits of the controller
 * command register. They tell the controller what sort
 * of operation we want to perform (see board schematic) */
#define CNTRBIT_LOAD_TOGGLE 	0x0001
#define CNTRBIT_RUN     	0x0002
#define CNTRBIT_SINGLE		0x0004
#define CNTRBIT_DIAG  		0x0008

/* !!! the following 7 bits control output enables of chips in the
 * kestrel array.  It is possible to create drive fights on the board
 * which can damage the hardware, so don't mess with these unless tou
 * are very familiar with the board design!!!
 */

/* output enable of banks of kestrel PEs 
 * for prototypes 1=disabled, 0=enabled
 * for 64 PE chipds, 1=enables, 0=both outputs on due to a hardware
 * bug never set any of these bits to 0 with 64 PE chips in the board
 * because it creates a drive fight that could damage our very
 * expensive chips.  
 */
#define CNTRBIT_K0_ENABLE	0x0010
#define CNTRBIT_K1_ENABLE	0x0020
#define CNTRBIT_K23_ENABLE	0x0040
#define CNTRBIT_K4567_ENABLE	0x0080
#define CNTRBIT_ALL_ENABLES     0x00F0

/* output enable of bypass buffer (1=disables, 0=enables */
#define CNTRBIT_BUF0_ENABLE	0x0100
#define CNTRBIT_BUF1_ENABLE	0x0200
#define CNTRBIT_BUF2_ENABLE	0x0400

/* a 0->1 transition on this bit causes the controller to pop * the
 * input queue. */
#define CNTRBIT_QIN_LOAD 	0x0800

/* a 0->1 transition causes three saved values to get written to qout */
#define CNTRBIT_RBQOUT		0x1000

/* these three commands are currently unused */
#define CNTRBIT_COMMAND_3	0x2000
#define CNTRBIT_COMMAND_4	0x4000

/* Reset the FPGA (0=reset mode, 1=normal mode) */
#define CNTRBIT_FPGA_RESET	0x8000

/* this mask is always or'd with the command register when written */
#define CNTRBIT_BOARD_ENABLE_MASK	(CNTRBIT_FPGA_RESET|CNTRBIT_BUF0_ENABLE|CNTRBIT_BUF1_ENABLE|CNTRBIT_BUF2_ENABLE)

/* The state of CNTRBIT_LOAD_TOGGLE is unimportant, it only
 * causes activity if changed in SINGLE, LOAD_INS, READ_INS,  and
 * PROG_FIFO modes to indicate to the controller that the 
 * operation configured by the host (us) should be performed.
 * (that is, the controller only detects changes in this bit)
 * 
 * These state are defined definitely in the controller schematics.
 */
#define CNTR_MODE_STOP 		0x0000
#define CNTR_MODE_RUN		CNTRBIT_RUN
#define CNTR_MODE_SINGLE	CNTRBIT_SINGLE
#define CNTR_MODE_LOAD_INS	(CNTRBIT_SINGLE|CNTRBIT_RUN)
#define CNTR_MODE_READ_INS	(CNTRBIT_SINGLE|CNTRBIT_DIAG)
#define CNTR_MODE_PROG_FIFO	(CNTRBIT_RUN|CNTRBIT_SINGLE|CNTRBIT_DIAG)
#define CNTR_MODE_DIAG		CNTRBIT_DIAG

#define CNTR_MODE_TERMINATE	(CNTRBIT_BOARD_ENABLE_MASK & ~(CNTRBIT_FPGA_RESET))

/* status register bits */
/* the lower eight bits are the diagnostic output bits;
 * these are the settings for non-diagnostic mode
 */
#define STATUS_REG_COUNTER_OVERFLOW	0x0001
#define STATUS_REG_COUNTER_UNDERFLOW	0x0002
#define STATUS_REG_PC_OVERFLOW		0x0004
#define STATUS_REG_PC_UNDERFLOW		0x0008
#define STATUS_REG_BREAKPOINT		0x0010
/* the next three bits are always zero */

#define STATUS_REG_QOUT_FULL		0x0100
#define STATUS_REG_QOUT_ALMOST_FULL	0x0200
#define STATUS_REG_QIN_EMPTY		0x0400
#define STATUS_REG_QIN_ALMOST_EMPTY	0x0800
#define STATUS_REG_LOAD_DONE		0x1000
#define STATUS_REG_DONE			0x2000
#define STATUS_REG_VALID_NEXT_DATA      0x4000
#define STATUS_REG_O_STATUS_2		0x8000

/* FIFO Full/Empty warning levels */
#define QIN_ALMOST_EMPTY	1024
#define QOUT_ALMOST_FULL	1024

#define FIFO_SERVE_FULL	  1
#define FIFO_SERVE_ALMOST 2
#define FIFO_SERVE_LAST   3

/*****************
 *Some Constant Addresses for the devices on the board are defined here
 ****************/
#define STATUS_REG_ADDRESS 	0
#define QIN_ADDRESS 		1
#define QOUT_ADDRESS 		2
#define CMD_REG_ADDRESS 	3
#define T1_ADDRESS 		4
#define T2_ADDRESS 		5
#define T3_ADDRESS 		6
#define PROG_FIFO_ADDRESS 	7
#define PROG_DELAY_LINES 	8
#define ENABLE_INTERRUPTS 	9
#define FLUSH_QUEUES 		10
#define READ_QOUT 		11
#define WRITE_QIN 		12
#define KCMD_SPECIFY_PROGRAM	13
#define KCMD_SPECIFY_QIN       	14
#define KCMD_SPECIFY_QOUT      	15
#define KCMD_RETRIEVE_QOUT     	16
#define KCMD_PROG_QIN	       	17
#define KCMD_PROG_QOUT	       	18
#define KCMD_RUN_PROGRAM       	19
#define KCMD_GET_ERROR_CODE    	20
#define KCMD_TERMINATE_PROGRAM	21

#define QIN_SIZE 4096
#define QOUT_SIZE 4096

/* value for action argument in RunningKestrel */
#define ACTION_RUN 1
#define ACTION_STEP 2

typedef struct _controller_state {
  int pc;
  int cntrstack[16];
  int cntrstack_el;
  int pcstack[16];
  int pcstack_el;
  unsigned char scratch;
  unsigned char cbs;
} ControllerState;

extern int CommandReg;
extern int GotIRQDuringRead;

void CheckProgrammedFPGA(void);
int ReadDataFromKestrel(void);
void DecodeStatusRegister(int value);
int RunningKestrel(UserProgram *program, int action);
char *Put_in_Pipe(int address, int rowr);
unsigned int GetStatusRegister(void);
void PutCommandReg(int mode, int load_toggle);
int GetControllerPC(void);
void SetControllerPC(int new_pc);
void WriteInstrToTransceivers(char *instr);
int LoadInstructions(char **instructions, int num_instr, int location);
void Kill_Kestrel(void);
int Step(UserProgram *program);
int First_Step(UserProgram *program);
void SaveControllerState(UserProgram *program);
void RestoreControllerState(UserProgram *program);
int ProcessStatusRegister(UserProgram *program, int state);
int KestrelRead(unsigned char *storage, int byte_to_read);
int KestrelWrite(unsigned char *storage, int bytes_to_write);
void InitializeBoard(void);
void WriteDataToKestrel(int value);

#endif
