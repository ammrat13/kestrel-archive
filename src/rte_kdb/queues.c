/* $Id: queues.c,v 1.14 1999/01/09 01:07:18 leslie Exp $ */
/* 
 * Kestrel Run Time Environment
 * opyright (C) 1998  Regents of the University of California.
 *
 * queues.c - routines that maintain the data in the kestrel input
 *	      and output queues using the controller and pci interface.
 */

#include "globals.h"
#include "pe.h"
#include "interface.h"
#include "program.h"
#include "code.h"
#include "getstate.h"

int q_out_was_empty = 1;


/* 
 * Function   : SetQInAlmostEmptyTrigger
 * Purpose    : Reconfigure the almost empty trigger level of the FIFO chip
 *		on the Kestrel board.  This function may be called at any time
 * 		regardless of the state of the controller or FIFOs.
 * Parameters : value - new value to place in FIFO almost-empty-trigger-level 
 *			register.
 * Returns    : nothing
 * Notes      : This function writes to the programmable flag registers of
 *		the QIn side of the CY48X1 FIFO chip on the kestrel board.
 *		The flag register is a four byte value that defines the
 *		almost empty and almost full threashold values for that FIFO
 *		(the QOut side has an independent set of registers).  The
 *		values are written sequentially to the registers automatically
 *		by the FIFO in the following order:
 *		0: LSB of almost empty threashold (8 bits)
 *		1: MSB of almost empty threashold (4 bits for 4K FIFO)
 *		2: LSB of almost full threashold (8 bits)
 *		3: MSB of almost full threashold (4 bits for 4K FIFO)
 *
 * 	 	The almost empty signal (_PAEA) is asserted when the
 * 	 	number of elements in the FIFO is between zero and
 * 	 	the number in the almost empty register, inclusive.
 *
 *		The almost full signal (_PAFA) is unconnected in the
 *		current board; the contents of the first two registers
 *		is irrelevant.
 *
 * 		See the CY48X1 data sheet for more details.  
 */
void SetQInAlmostEmptyTrigger(int value)
{
  write(write_to_kestrel, Put_in_Pipe(PROG_FIFO_ADDRESS, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(value & 0xff, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(PROG_FIFO_ADDRESS, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe( (value >> 8) & 0xff, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(PROG_FIFO_ADDRESS, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(PROG_FIFO_ADDRESS, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);
}

/* 
 * Function   : SetQOutAlmostFullTrigger
 * Purpose    : Reconfigure the almost full level of the output queue
 *		on the FIFO chip on the kestrel board.  Programming the
 *		output queue level is more complex than the input
 *		queue because we can only write to the output queue
 *		from the controller using data from the input queue.
 *		Thus, this function can only be called after the state
 *		of the controller has been preserved (scratch register saved)
 *		and the input queue is empty.
 * Parameters : value - 12-bit value to place in the QOut FIFO 
 *		        almost-full-trigger-level register.
 * Returns    : nothing
 * Notes      : The output queue of the CY48X1 FIFO chip has the same
 *		trigger-level reigsters as the input queue side.  Here
 *		we want to write to the almost full level (the first two
 *		registers).  The values to write to the configuration registers
 *		are loaded into the input queue, read into the controller
 *		scratch register, and then written to the output queue
 *		in the controller's program-fifo mode by flipping the
 *		LOAD_TOGGLE bit in the command register.  The controller's
 *		program fifo mode works exactly like diagnistic mode except
 *		data written to the output queues goes into the fifos
 *		programmable registers.
 *
 *		The almost full signal (_PAFB) is a asserted when the number
 *		of elements in the output queue is between 4096-m and 4096,
 *		inclusive, where m is the number in the almost-full-trigger-level
 *		register.
 *
 *		The almost empty signal (_PAEB) is unconnected in the current
 *		board design; the contents of the last two registers is
 *		irrelevant.
 *
 * 		See the CY48X1 data sheet and the controller documentation
 * 		for more details.  
 */
void SetQOutAlmostFullTrigger(int value)
{
  char *instr;

  PutCommandReg(CNTR_MODE_STOP, 0);

  write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(value & 0xff, 0), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe((value >> 8) & 0xf, 0), PIPE_WIDTH);
  PutCommandReg(CNTRBIT_QIN_LOAD, 0);
  PutCommandReg(CNTR_MODE_STOP, 0);

  instr = MakeControllerQinToQoutInstr();
  WriteInstrToTransceivers(instr);
  PutCommandReg(CNTR_MODE_PROG_FIFO, 0);
  PutCommandReg(CNTR_MODE_PROG_FIFO, 1);
  PutCommandReg(CNTR_MODE_PROG_FIFO, 1);
  PutCommandReg(CNTR_MODE_PROG_FIFO, 1);
  PutCommandReg(CNTR_MODE_PROG_FIFO, 1);

  PutCommandReg(CNTR_MODE_STOP, 0);
}


/* 
 * Function   : ReadQOutElement
 * Purpose    : Read an element from the output queue.
 * Parameters : none
 * Returns    : byte value read
 * Notes      : 
 */
int ReadQOutElement(void)
{
  write(write_to_kestrel, Put_in_Pipe(QOUT_ADDRESS,READ), PIPE_WIDTH);
  return ReadDataFromKestrel();
}


/* 
 * Function   : ReadQout
 * Purpose    : Reads elements from the output queue and saves it in
 *		the specified UserProgram structure.  The number of
 *		elements to read from the FIFO is specified with the
 *		type field which indicates the event that triggered
 *		the call to this function (QOut full or almost full).
 * Parameters : program - UserProgram structure to store output data.
 *              type - if FIFO_SERVE_FULL => QOUT_SIZE bytes read
 *	               if FIFO_SERVE_ALMOST => QOU_SIZE - QOUT_ALMOST_FULL bytes read.
 * Returns    : boolean success/failure
 * Notes      : In the current board design, the almost empty signal of the
 *		output queue is unconnected, so the only way to determine
 *		when all the elements from the output queue have been
 *		read is to reprogram the almost full signal.  Reprogramming
 *		the almost full signal requires stopping the controller and
 *		doing lots of data movement in the controller (see FlushQueues)
 *		and would cause an unacceptable slowdown of program execution.
 *		Thus, we can only read out the number of bytew we
 *		are absolutely sure are present in the output queue.
 *
 *		Future board designs will have the FIFO output queue empty signal
 *		connected to the status register!
 */
int ReadQout(UserProgram *program, int type)
{ 
  unsigned int value;
  int count = 0;
  int max_count;

  if (type == FIFO_SERVE_FULL) {
    max_count = QOUT_SIZE;
  } else if (type == FIFO_SERVE_ALMOST) {
    max_count = QOUT_SIZE - QOUT_ALMOST_FULL;
  } else {
    ErrorPrint("ReadQout", "bad type field specified.");
    return (0);
  }

  if (q_out_was_empty == 1) {
    ReadQOutElement();
  }
  if ( 0 /* have_board == 0 */) {
    while (count < max_count) {
      value = ReadQOutElement();
      if (!PutDataInOutputBuffer(program, (unsigned char)value)) {
	ErrorPrint("ReadQout", "PutDataInOutputBuffer failed.");
	return (0);
      }
      count++;
    }
  } else {
    unsigned char *buffer;

    write(write_to_kestrel, Put_in_Pipe(READ_QOUT, WRITE), PIPE_WIDTH);
    write(write_to_kestrel, Put_in_Pipe(max_count, 0), PIPE_WIDTH);
    buffer = malloc(max_count);

    KestrelRead(buffer, max_count);

    for (count = 0; count < max_count; count++) {
      if (!PutDataInOutputBuffer(program, buffer[count])) {
	ErrorPrint("ReadQout", "PutDataInOutputBuffer failed.");
	return (0);
      }
    }

    free(buffer);
  } 

  if (type == FIFO_SERVE_FULL) {
    PutCommandReg(CNTR_MODE_STOP, 0);
    PutCommandReg(CNTRBIT_RBQOUT, 0);
    PutCommandReg(CNTR_MODE_STOP, 0);
    q_out_was_empty = 1;
  } else {
    q_out_was_empty = 0;
  }

  return 1;
} 


/* 
 * Function   : LoadQin
 * Purpose    : Loads data into the input queue from the specified
 *		UserProgram structure.
 * Parameters : program - current UserProgram structure
 *		type - event specifier that indicates what triggered
 *		       the call to this function.
 *			FIFO_SERVE_FULL - load at most QIN_SIZE bytes into
 *			into QIn.  The controller must be in stop mode before
 *			the function is called in this case.
 *			FIFO_SERVE_EMPTY - load at most QIN_SIZE -
 *			 QIN_ALMOST_EMPTY bytes into QIn.
 * Returns    : boolean success/failure
 * Notes      : Similarly to the output queue, there is no way
 *		to detect when the input queue is full, as the full
 *		signal for the input queue is unconnected in the current
 *		board design.  Consequently, we can load only the
 *		data elements we are absolutely sure will fit in the
 *              FIFO to avoid losing data.  The rte can determine how many
 *		elements are in the input queue by stopping the controller
 *		and reading the input queue into the output queue one
 *		element at a time, but this would cause an unacceptable 
 * 		performance penalty during normal program execution.
 *
 *		Another sticky point is determining when the user
 *		did not provided enough data versus when they provided 
 * 		exactly the right amount.  The problem is when the
 *		program exhausts its input data the input queue empty
 *		flag is set in the status register; if the
 *		program does not need more data then an interrupt
 *		will not occur and controller execution will continue,
 *		ignoring the empty signal from the fifo.  When the next
 *		interrupt from the controller occurs (QOut level,
 *		breakpoint, etc), the rte will check the status register
 *		and find that QIn is empty, but won't know if the 
 *		controller was actually trying to read data from
 *		the input queue.  The solution to this problem is to
 *		pad the input data with an extra byte so if the rte
 *		discovers an empty QIn when all the data plus the
 *		extra byte has been put in queue, it will be absolutely
 *		sure the program doesn't have enough data.
 */
int LoadQin(UserProgram *program, int type)
{
  static unsigned char buffer[5000];
  unsigned char data;
  int count = 0, iteration;
  int max_move, give_last_byte;

  if (type == FIFO_SERVE_FULL) {
    max_move = QIN_SIZE; 
  } else if (type == FIFO_SERVE_ALMOST) { /* queue in almost empty level */
    max_move = QIN_SIZE - QIN_ALMOST_EMPTY;
  }

  if (program->input_recoveredFromQin) {
    /* data from the input queue recovered by FlushQueues */
    /* assume this data will fit in queue (we are restoring the previous contents) */
    for (count = 0; count < program->input_numQinBytes; count++) {
      buffer[count] = program->input_recoveredFromQin[count];	
    }
    free(program->input_recoveredFromQin);
    program->input_recoveredFromQin = NULL;
    program->input_numQinBytes = 0;
    goto write_data;
  }

  /* detect when the input data is exhausted */
  if (type == FIFO_SERVE_FULL && IsInputDataExhausted(program)) {
    /*    sprintf(print_msg, "%s does not have enough data for program %s\n",
	  program->input_fileName, program->code_fileName); */
    /*
printf("%d %d %d %d %d\n",program->useMemoryInput,
program->input_mem_current_index,
program->input_total,
program->input_gaveLastByte,
program->input_qinpad);
    */
    sprintf(print_msg, "yMULTI FILES, does not have enough data for program %s\n",
	     program->code_fileName);
    ScreenPrint(print_msg);
#if 0
    sprintf(print_msg, "program read past end of input stream on or after instruction %d\n",
	    program->input_gaveLBInstr);
#endif
    ScreenPrint(print_msg);
    return (0);
  }

  if (type == FIFO_SERVE_FULL) {
    give_last_byte = 1;
  } else {
    give_last_byte = 1 /* 0 */;
  }
  while (count < max_move && GetNextInputData(program, &data, give_last_byte) ) {
    buffer[count++] = data;
    /*     give_last_byte = 0; */
  }
  /*printf("count=%d %d %d\n",count, max_move, program->input_mem_current_index);
   */
write_data :
  /* count holds number of bytes to write to qin from buffer */
  if (have_board == 0) {
    for (iteration = 0; iteration < count; iteration++) {
      data = buffer[iteration];
      write(write_to_kestrel, Put_in_Pipe(QIN_ADDRESS,WRITE), PIPE_WIDTH);
#if DEBUG
      sprintf(print_msg, "sending QIn: %u\n", (unsigned int)data);
      ScreenPrint(print_msg);
      fflush(stdout);
#endif
      write(write_to_kestrel, Put_in_Pipe(data, 0), PIPE_WIDTH);
    }
  } else {
    unsigned char cmd_buf[10];
    write(write_to_kestrel, Put_in_Pipe(WRITE_QIN,WRITE), PIPE_WIDTH);
    write(write_to_kestrel, Put_in_Pipe(count, 0), PIPE_WIDTH);
    KestrelRead(cmd_buf, 9);
    if (strcmp((char *)cmd_buf, SYNC_SIGNAL)) {
	ErrorPrint("LoadQin", "server synchronization failure during block write\n");
	return 0;
    }
    KestrelWrite(buffer, count);
  }

  if (type == FIFO_SERVE_FULL) {
    /* a 0->1 transistion of the CNTRBIT_QIN_LOAD bit of the command reigster
     * causes the controller to load the first data element from the fifo.
     * This is necessary because the first element loaded into the queue does
     * not appear on the output pins until after the first pop. This initial pop
     * is necessary because of the details of the read timing of the fifo by
     * the controller. 
     */
    PutCommandReg(CNTR_MODE_STOP, 0);
    PutCommandReg(CNTRBIT_QIN_LOAD, 0);
    PutCommandReg(CNTR_MODE_STOP, 0);
  }

  return (1);
}

/* 
 * Function   : FlushQueues
 * Purpose    : Empty both the input and output queues.
 * Parameters : program - UserProgram structure for the current program/
 * Returns    : boolean success/failure.
 * Notes      : This routine is flushes an unknown number of
 *		data elements from the input and output queue. First, it
 *		reads data from the input to the output queues using
 *		the QIn empty signal. Next, the almost full signal of
 *		the output FIFO is reprogrammed to become a not-empty
 *		signal, allowing the routine to retrieve the correct
 *		number of elements from the output queue.
 */
int FlushQueues(UserProgram *program)
{
  char *instr;	
  unsigned char *storage;
  int position, status, old_status, index;
  int qin_data, qout_data;

  if (program->queues_flushed == 1) {
    return 1;
  } 
  program->queues_flushed = 1;

  SaveControllerState(program);

  PutCommandReg(CNTR_MODE_STOP, -1);

  storage = malloc(sizeof(unsigned char) * 2 * QIN_SIZE);
  if (storage == NULL) {
    ErrorPrint("FlushQueues", "Out of memory.");
    return (0);
  }

  if (have_board == 0) {
    PutCommandReg(CNTR_MODE_DIAG, 0);

    instr = MakeControllerQinToQoutInstr();
    WriteInstrToTransceivers(instr);

    /* empty input queue into the output queue */
    qin_data = 0;
    position = 0;
    status = GetStatusRegister();
    while (!(status & STATUS_REG_VALID_NEXT_DATA)) {
      if (status & STATUS_REG_QOUT_FULL) {
	if (q_out_was_empty == 1) {
	  ReadQOutElement();
	}
	for (index = 0; index < QOUT_SIZE; index++) {
	  storage[position++] = ReadQOutElement();
	}
	q_out_was_empty = 1;
      }
      PutCommandReg(CNTR_MODE_DIAG, 1);
      status = GetStatusRegister();
      qin_data ++;
    }

    /* reprogram the output queue almost-full level */
    SetQOutAlmostFullTrigger(QOUT_SIZE - 1);

    /* read elements from the output queue until it is empty */
    status = old_status = GetStatusRegister();
    if (q_out_was_empty == 1) {
      ReadQOutElement();
    }
    while (1) {
      if (!(status & STATUS_REG_QOUT_ALMOST_FULL) &&
	  !(old_status & STATUS_REG_QOUT_ALMOST_FULL)) {	
	break;
      }
      storage[position++] = ReadQOutElement();
      old_status = status;
      status = GetStatusRegister();
    }
  } else {
    write(write_to_kestrel, Put_in_Pipe(FLUSH_QUEUES,WRITE),PIPE_WIDTH);
    write(write_to_kestrel, Put_in_Pipe(q_out_was_empty,0),PIPE_WIDTH);

    position = ReadDataFromKestrel();
    qin_data = ReadDataFromKestrel();

    KestrelRead(storage, position);
  }

  qout_data = position - qin_data;
  if (position < qin_data) {
    sprintf(print_msg, 
	    "wrong number of elements read from qout (%d, should be at least %d)",
	    position, qin_data);
    ErrorPrint("FlushQueues", print_msg);
  }

  PutCommandReg(CNTR_MODE_STOP, -1);

  /* restore the output queue almost-full trigger level */
  SetQOutAlmostFullTrigger(QOUT_ALMOST_FULL);

  RestoreControllerState(program);

  q_out_was_empty = 1;

  printf("QIN DATA: %d QOUT DATA: %d\n", qin_data, qout_data);

  /* store the data from the input queue in a place where the next
   * call to LoadQin can find */
  if (qin_data > 0) {
    if (program->input_recoveredFromQin != NULL) {
      ErrorPrint("FlushQueues", "program->input_recoveredFromQin already had data!\n");
      return (0);
    }

    if ((program->input_recoveredFromQin = malloc(qin_data)) == NULL) {
      ErrorPrint("FlushQueues", "Out of memory.");
      return (0);
    }
    for (index = 0; index < qin_data; index++) {
      program->input_recoveredFromQin[index] = storage[qout_data + index];
    }
    program->input_numQinBytes = qin_data;
  }

  /* save the data from the output queue */
  for (index = 0; index < qout_data; index++) {
    if (!PutDataInOutputBuffer(program, storage[index])) {
      sprintf(print_msg, "PutDataInOutputBuffer failed.");
      ErrorPrint("FlushQueues", print_msg);
      return (0);
    }
  }

  free(storage);

  return (1);
}

/* end of file 'queues.c' */
