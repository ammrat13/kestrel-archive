/*kesboard.c
 *
 *Justin Meyer justin@cse.ucsc.edu
 *
 *The implementation file for the various  functions and such in 
 *kesboard.h
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "kesboard.h"

#include "kestrel.h" 

#define DEBUG 0

/************************************************************************
 * Global Data
 *************************************************************************/

/*the current instruction*/
Instruction instruction;

/*Transeivers*/
char Transceiver[25];

/* Input and Output Queues */
unsigned char QIn[Q_IN_SIZE];
unsigned char QInTOSReg;
unsigned char QOut[Q_OUT_SIZE];
int QInHead;
int QInTail;
int QInBytes;
int QOutHead;
int QOutTail;
int QOutBytes;

int QInTOSRegEmpty;
int QOutTOSReg;

/*the array that is program memory*/
char programMem[PROG_SIZE][25]; 
int write_index;

/*declaration of the status register*/
StatusReg Status_Register;

/*The command register is represented as a single int.*/
int CommandReg, OldCommandReg; 

/* fifo almost full/empty conditions */
int QOutAlmostFullLevel = QOUT_ALMOST_FULL_DEFAULT;
int QInAlmostEmptyLevel = QIN_ALMOST_EMPTY_DEFAULT;
int WriteQOutLevelPtr = 0;
int WriteQInLevelPtr = 0;


/************************************************************************
 * Local Data
 ***********************************************************************/

char bitstring[96];

/************************************************************************
 *The Board
 *Functions for proccessing the simulation of the various devices on the 
 *board
 ***********************************************************************/

/* These functions implement circular queues for input/output of
 * data to/from the Kestrel array.
 *
 * Head => location to read next character.
 * Tail => location to put next character.
 * Tail  == Head => Queue FULL
 * Head == -1 => Queue EMPTY
 *
 * FUTURE WORK: Add code to properly handle queue empty/full
 *              conditions (generate interrupts/stop execution).
 */

int IsQInEmpty(void)
{
  return (QInBytes == 0);
}

int IsQInAlmostEmpty(void)
{
  return (QInBytes <= QInAlmostEmptyLevel);
}

int QInPutData(unsigned char data)
{
  if (QInBytes == Q_IN_SIZE) {
    /* Buffer is full; FIFO output holds last read value until a new
     * value is available */
    fprintf(stderr, "kestrel sim: WARNING: Qin Full\n");
    fflush(stderr);
    return 1;
  }

  /* Add character to the buffer */
  QIn[QInTail] = data;
  QInTail = (QInTail + 1) % Q_IN_SIZE;
  if (QInBytes == 0) {
    QInHead = 0;
  }
  QInBytes ++;
  
  Status_Register.i_almost_empty = IsQInAlmostEmpty();
  Status_Register.i_empty = IsQInEmpty();

  return 1;
}

int QInGetData(unsigned char *data)
{
#if 0
  if (QInBytes == 0 && Status_Register.valid_next_data == 0) {
    /* Buffer is empty */
    fprintf(stderr, "kestrel sim: WARNING: QIn Empty\n");
    fflush(stderr);
  }
#endif

  /* get character from the buffer */
  *data = QInTOSReg;
  if (QInBytes > 0) {
    QInTOSReg = QIn[QInHead];
    QInHead = (QInHead + 1) % Q_IN_SIZE;
    QInBytes --;
    if (QInBytes == 0) {
      QInHead = -1;
      QInTail = 0;
    }
    Status_Register.valid_next_data = 1;
  } else {
    Status_Register.valid_next_data = 0;
  }

  if (IsQInAlmostEmpty()) {
    if (Status_Register.i_almost_empty == 0) {

      Status_Register.i_almost_empty = 1;
      TriggerIRQ();
    }
  } 
  Status_Register.i_empty = IsQInEmpty();

  return 1;
}

int IsQOutFull(void)
{
  return (QOutBytes == Q_OUT_SIZE);
}

int IsQOutAlmostFull(void)
{
  return (QOutBytes >= Q_OUT_SIZE - QOutAlmostFullLevel);
}

int QOutPutData(unsigned char data)
{
  if (QOutBytes == Q_OUT_SIZE) {
    /* Buffer is full */
    fprintf(stderr, "\nkestrel sim: WARNING: Write to full QOut\n");
    fflush(stderr);
    return 1;
  } else {
    /* Add character to the buffer */
    QOut[QOutTail] = data;
    QOutTail = (QOutTail + 1) % Q_OUT_SIZE;
    if (QOutBytes == 0) {
      QOutHead = 0;
    }
    QOutBytes++;

    if (IsQOutAlmostFull()) {
      if (Status_Register.o_almost_full == 0) {
	Status_Register.o_almost_full = 1;
	TriggerIRQ();
      }
    } 
    if (IsQOutFull()) {
      Status_Register.o_full = 1;
      TriggerIRQ();
    }

    return 1;
  }
}

int QOutGetData(unsigned char *data)
{
  if (QOutBytes == 0) {
    *data = QOutTOSReg;
  } else {
    /* get character from the buffer */
    *data = QOutTOSReg;
    QOutTOSReg = QOut[QOutHead];
    QOutHead = (QOutHead + 1) % Q_OUT_SIZE;
    QOutBytes --;
    if (QOutBytes == 0) {
      QOutHead = -1;
      QOutTail = 0;
    }

    Status_Register.o_almost_full = IsQOutAlmostFull();
    Status_Register.o_full = IsQOutFull();
  }

  return 1;
}

void print_queue(char in_out)
{
  int i;
  if(in_out == 'i') {
    fprintf( stderr, "sim: QIn:\n" );
    fprintf( stderr, "QInHead == [%d], QInTail == [%d] QInBytes == [%d] \
valid_next_data: %d i_almost_empty: %d\n", 
	     QInHead, QInTail, QInBytes, Status_Register.valid_next_data,
	     Status_Register.i_almost_empty);
    if (QInHead != -1) {
      for(i = QInHead; i != QInTail; i = (i + 1) % Q_IN_SIZE) {
	fprintf(stderr, "%d, ", QIn[i] );
      } 
      fprintf( stderr, "\n\n");
    }
  } else if(in_out == 'o') {
    fprintf( stderr, "sim: QOut:\n" );
    fprintf( stderr, "sim: QOutHead == [%d], QOutTail == [%d], QOutBytes == [%d] \
o_full: %d, o_almost_full: %d\n", 
	     QOutHead, QOutTail, QOutBytes, Status_Register.o_full,
	     Status_Register.o_almost_full);
    if (QOutHead != -1) {
      for(i = QOutHead ; i != QOutTail ; i = (i + 1) % Q_OUT_SIZE) {
	fprintf(stderr, "%d, ", QOut[i] );
      }
      fprintf( stderr, "\n\n");
    }
  }

  fflush(stderr);
}


int CheckCommandRegMode(int mode)
{
  int mask_cmdreg;

  mask_cmdreg = CommandReg & (CNTRBIT_RUN|CNTRBIT_SINGLE|CNTRBIT_DIAG);

  return ((mask_cmdreg & mode) == mode);
}

pcidata_ptr Process_PCI_data(int foo)
{
  static pcidata p;
  p.read_or_write = foo >> 31;
  p.address = foo & 0x7fffffff;
  return &p; 
}

void PrintInstruction(void)
{
  fprintf(stderr, "pc_out_sel %d push_pc %d pop_pc %d push_cnt %d dec_cnt %d\n",
	  instruction.pc_out_sel,
	  instruction.push_pc,
	  instruction.pop_pc,
	  instruction.push_cnt,
	  instruction.dec_cnt);
  fprintf(stderr, "br_0 %d br_w_or %d cntr_write %d cbs_load %d cbs_sleft %d\n",
	  instruction.br_0,
	  instruction.br_w_or,
	  instruction.cntr_write,
	  instruction.cbs_load,
	  instruction.cbs_sleft);
  fprintf(stderr, "scr_sel %d scr_store %d in_data_sel %d data_read %d\n",
	  instruction.scr_select,
	  instruction.scr_store,
	  instruction.in_data_sel,
	  instruction.data_read);
  fprintf(stderr, "data_write %d fifo_out %d brk %d con_imm %d d_pop_cnt %d d_out %d\n",
	  instruction.data_write,
	  instruction.fifo_out,
	  instruction.break_bit,
	  instruction.cont_immediate,
	  instruction.d_pop_counter,
	  instruction.d_out);
  fflush(stderr);
}


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
    printf("hextoint: Bad Hexidecimal value.");
    return -1;
  }
  return value;
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

int GenerateBinaryInstr(char *instr, char *bitstring)
{
  int index, value, power;

  for(index = 0; index < 24; index++) {
    if ((value = hextoint(instr[index])) == -1) {
      printf("Bad Instruction: %s\n", instr);
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

int Interpret_Inst(void)
{
  static char bitstring[97];

  if (!GenerateBinaryInstr(Transceiver, bitstring)) {
    return 0;
  }

  instruction.pc_out_sel = GetInstructionField(bitstring, PC_OUT_SEL, PC_OUT_SELSIZE);
  instruction.push_pc = GetInstructionField(bitstring, PUSH_PC, PUSH_PCSIZE);
  instruction.pop_pc = GetInstructionField(bitstring, POP_PC, POP_PCSIZE);
  instruction.push_cnt = GetInstructionField(bitstring, PUSH_CNT, PUSH_CNTSIZE);
  instruction.dec_cnt = GetInstructionField(bitstring, DEC_COUNT, DEC_COUNTSIZE);
  instruction.br_0 = GetInstructionField(bitstring, BR_0, BR_0SIZE);
  instruction.br_w_or =GetInstructionField(bitstring, BR_W_OR, BR_W_ORSIZE);
  instruction.cntr_write = GetInstructionField(bitstring, CNTR_WRITE, CNTR_WRITESIZE);
  instruction.cbs_load = GetInstructionField(bitstring, CBS_LOAD, CBS_LOADSIZE);
  instruction.cbs_sleft = GetInstructionField(bitstring, CBS_SLEFT, CBS_SLEFTSIZE);
  instruction.scr_select = GetInstructionField(bitstring, SCR_SEL, SCR_SELSIZE);
  instruction.scr_store = GetInstructionField(bitstring, SCR_STORE, SCR_STORESIZE);
  instruction.in_data_sel = GetInstructionField(bitstring, IN_DATA_SEL, IN_DATA_SELSIZE);
  instruction.unused_0 = GetInstructionField(bitstring, UNUSED_0, UNUSED_0SIZE);
  instruction.data_read = GetInstructionField(bitstring, DATA_READ, DATA_READSIZE);
  instruction.data_write = GetInstructionField(bitstring, DATA_WRITE, DATA_WRITESIZE);
  instruction.fifo_out = GetInstructionField(bitstring, FIFO_OUT, FIFO_OUTSIZE);
  instruction.break_bit = GetInstructionField(bitstring, BREAK, BREAKSIZE);
  instruction.cont_immediate = GetInstructionField(bitstring, CONT_IMM, CONT_IMMSIZE);
  instruction.d_pop_counter = GetInstructionField(bitstring, D_POP_CNTR, D_POP_CNTRSIZE);
  instruction.d_out = GetInstructionField(bitstring, D_OUT, D_OUTSIZE);
  instruction.board_imm_mux = GetInstructionField(bitstring, BOARD_IMM_MUX, BOARD_IMM_MUXSIZE);
  instruction.spare = GetInstructionField(bitstring, SPARE, SPARESIZE);
  instruction.force = GetInstructionField(bitstring, FORCE, FORCESIZE);
  instruction.func = GetInstructionField(bitstring, FUNC, FUNCSIZE);
  instruction.ci = GetInstructionField(bitstring, CI, CISIZE);
  instruction.mp = GetInstructionField(bitstring, MP, MPSIZE);
  instruction.lc = GetInstructionField(bitstring, LC, LCSIZE);
  instruction.finv = GetInstructionField(bitstring, FINV, FINVSIZE);
  instruction.fb = GetInstructionField(bitstring, FB, FBSIZE);
  instruction.bit = GetInstructionField(bitstring, BIT, BITSIZE);
  instruction.rm = GetInstructionField(bitstring, RM, RMSIZE);
  instruction.rd = GetInstructionField(bitstring, RD, RDSIZE);
  instruction.wr = GetInstructionField(bitstring, WR, WRSIZE);
  instruction.opB = GetInstructionField(bitstring, OPB, OPBSIZE);
  instruction.opA = GetInstructionField(bitstring, OPA, OPASIZE);
  instruction.opC = GetInstructionField(bitstring, OPC, OPCSIZE);
  instruction.dest = GetInstructionField(bitstring, DEST, DESTSIZE);
  instruction.imm = GetInstructionField(bitstring, IMM, IMMSIZE);
  instruction.nop = 0;

  return (1);
}

char dec_to_hex(int number)
{
  if (number >= 0 && number <= 9) {
    return number + '0';
  } else if (number >= 10 && number <= 15) {
    return number + 'A' - 10;
  } else {
    printf("kestrel sim: dec_to_hex: bad value %d\n", number);
    return '0';
  }
}

char* Get_StatusReg(void)
{
  int value = 0;
  static char SR[10];

  ComputeDiagnosticOutput();

  /* the low order byte is always the diagnostic output.
   * if not in diagnostic mode, then this describes the state
   * of the controller, otherwise, it uses d_out to select an output
   */
  value = (Status_Register.diagnostic_data & 0xff) |
          ((Status_Register.o_full == 0) << 8) |
          ((Status_Register.o_almost_full == 0) << 9) |
 	  ((Status_Register.i_empty == 0) << 10) |
          ((Status_Register.i_almost_empty == 0) << 11) |
          ((Status_Register.valid_next_data == 0) << 14);
  sprintf(SR, "%08x", value);

#if DEBUG
  fprintf(stderr, "(%s) diag: %d o_f: %d o_a_f: %d i_e: %d i_a_e: %d v_n_d: %d\n",
	  SR,
	  Status_Register.diagnostic_data,
	  Status_Register.o_full,
	  Status_Register.o_almost_full,
	  Status_Register.i_empty,
	  Status_Register.i_almost_empty,
	  Status_Register.valid_next_data);

  fflush(stderr);
#endif

  return SR;
}

void Write_T_One(char* data)
{
  Transceiver[16] = data[0];
  Transceiver[17] = data[1];
  Transceiver[18] = data[2];
  Transceiver[19] = data[3];
  Transceiver[20] = data[4];
  Transceiver[21] = data[5];
  Transceiver[22] = data[6];
  Transceiver[23] = data[7];
}

void Write_T_Two(char* data)
{
  Transceiver[8] =  data[0];
  Transceiver[9] =  data[1];
  Transceiver[10] = data[2];
  Transceiver[11] = data[3];
  Transceiver[12] = data[4];
  Transceiver[13] = data[5];
  Transceiver[14] = data[6];
  Transceiver[15] = data[7];
}

void Write_T_Three(char* data)
{
  Transceiver[0] = data[0];
  Transceiver[1] = data[1];
  Transceiver[2] = data[2];
  Transceiver[3] = data[3];
  Transceiver[4] = data[4];
  Transceiver[5] = data[5];
  Transceiver[6] = data[6];
  Transceiver[7] = data[7];
}


char* Read_T_One(void)
{
  static char T1[9];

  T1[0] = Transceiver[16];
  T1[1] = Transceiver[17];
  T1[2] = Transceiver[18];
  T1[3] = Transceiver[19];
  T1[4] = Transceiver[20];
  T1[5] = Transceiver[21];
  T1[6] = Transceiver[22];
  T1[7] = Transceiver[23];
  T1[8] = 0;

  return T1;
}
char* Read_T_Two(void)
{
  static char T2[9];

  T2[0] = Transceiver[8];
  T2[1] = Transceiver[9];
  T2[2] = Transceiver[10];
  T2[3] = Transceiver[11];
  T2[4] = Transceiver[12];
  T2[5] = Transceiver[13];
  T2[6] = Transceiver[14];
  T2[7] = Transceiver[15];
  T2[8] = 0;

  return T2;
}

char* Read_T_Three(void)
{
  static char T3[9];	

  T3[0] = Transceiver[0];
  T3[1] = Transceiver[1];
  T3[2] = Transceiver[2];
  T3[3] = Transceiver[3];
  T3[4] = Transceiver[4];
  T3[5] = Transceiver[5];
  T3[6] = Transceiver[6];
  T3[7] = Transceiver[7];
  T3[8] = 0;
  
  return T3;
}

void Init_Board(void)
{
  int ind;
  for( ind = 0; ind < 24; ind++ )
    {
      Transceiver[ind] = 0;
    }
  for( ind = 0; ind < Q_IN_SIZE; ind++ )
    {
      QIn[ind] = 0;
    }
  for( ind = 0; ind < Q_OUT_SIZE; ind++ )
    {
      QOut[ind] = 0;
    }
  write_index = 0;

  QInHead = 0;
  QInTail = 0;
  QInBytes = 0;
  QOutHead = 0;
  QOutTail = 0;
  QOutBytes = 0;

  Status_Register.load_done  = 0;
  Status_Register.break_bit = 0;
  Status_Register.i_empty = 1; /* input queue is empty at init time */
  Status_Register.valid_next_data = 0;
  Status_Register.i_almost_empty = 1;  /* this too */
  Status_Register.o_full = 0;
  Status_Register.o_almost_full = 0;
  Status_Register.pc_stack_overflo = 0;
  Status_Register.pc_stack_underflo = 0;
  Status_Register.cntr_stack_overflo = 0;
  Status_Register.cntr_stack_underflo = 0;
  CommandReg = 0;
  OldCommandReg = 0;
}
  

void WriteQueueLevel(int type, int *level, int *counter, int new_val)
{
  if (type == MODIFY_LEVEL_QOUT) {
    switch(*counter) {
    case LEVEL_FULL_MSB :
      *level = (*level & 0x00ff) | ((new_val & 0x0f) << 8);
      break;

    case LEVEL_FULL_LSB :
      *level = (*level & 0x0f00) | (new_val & 0xff);
      break;
    }
  } else if (type == MODIFY_LEVEL_QIN) {
    switch(*counter) {
    case LEVEL_EMPTY_MSB :
      *level = (*level & 0x00ff) | ((new_val & 0x0f) << 8);
      break;

    case LEVEL_EMPTY_LSB :
      *level = (*level & 0x0f00) | (new_val & 0xff);
      break;
    }
  }
  *counter = (*counter + 1) % TOTAL_LEVELS;
}
