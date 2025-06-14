/*************************************************************************
 *
 *controller.c
 *
 *This file contains the functions and data structures that make Kestrel's 
 *Controller.
 *
 *created 1/18/97 Justin Meyer justin@cse.ucsc.edu
 *
 *************************************************************************/

#include "kestrel.h"
#include "kesboard.h"
#include "controller.h"

extern void Array_Execute(void);

#define DEBUG 0

Controller controller;

/************************************************************************
 *The Controller
 *
 *All the functions relatating to the controller appear below
 ************************************************************************/

void Init_Controller(void)
{
  int i;
  controller.pc = 0;
  controller.pcstack_el = 0;
  controller.cntrstack_el = 0;
  for( i = 0; i <= 15; i++){
    controller.pcstack[i] = 0;
  }
  for( i = 0; i <= 15; i++){
  controller.cntrstack[i] = 0;
  }
  controller.Scratch = 0;
  controller.cbs0 = 0;
  controller.cbs1 = 0;
  controller.cbs2 = 0;
  controller.cbs3 = 0;
  controller.cbs4 = 0;
  controller.cbs5 = 0;
  controller.cbs6 = 0;
  controller.cbs7 = 0;

  controller.ArrayDinMask = 0;
  controller.ArrayDoutMask = 0;
  controller.ArrayDataInLeft = 0;
  controller.ArrayDataInRight = 0;
  controller.ArrayDataOut = 0;
  controller.ArrayImmediate = 0;
  controller.qin_poped_data = 0;
  controller.poped_counter_stack = 0;
  controller.execution_halted = 0;

  controller.wor = controller.wor2 = 0;
}

void Load_CBS(void) 
{
   int temp;

   temp = (controller.wor >> 0);
   temp = (temp & 1); 
   controller.cbs0 = temp;
   temp = (controller.wor >> 1);
   temp = (temp & 1); 
   controller.cbs1 = temp;
   temp = (controller.wor >> 2);
   temp = (temp & 1); 
   controller.cbs2 = temp;
   temp = (controller.wor >> 3);
   temp = (temp & 1); 
   controller.cbs3 = temp;
   temp = (controller.wor >> 4);
   temp = (temp & 1); 
   controller.cbs4 = temp;
   temp = (controller.wor >> 5);
   temp = (temp & 1); 
   controller.cbs5 = temp;
   temp = (controller.wor >> 6);
   temp = (temp & 1); 
   controller.cbs6 = temp;
   temp = (controller.wor >> 7);
   temp = (temp & 1); 
   controller.cbs7 = temp;
}


void Shift_CBS(void)
{
  int temp;
  temp = (controller.wor || 0);
  controller.cbs7 = controller.cbs6;
  controller.cbs6 = controller.cbs5;
  controller.cbs5 = controller.cbs4;
  controller.cbs4 = controller.cbs3;
  controller.cbs3 = controller.cbs2;
  controller.cbs2 = controller.cbs1;
  controller.cbs1 = controller.cbs0;
  controller.cbs0 = temp;
}


int Push_PC(void)
{
  if (controller.pcstack_el == 15)
    {
      Status_Register.pc_stack_overflo = 1;
      TriggerIRQ();
      controller.execution_halted = 1;
      fprintf(stderr, "kestrel sim: PC stack overflow at %d.\n",
	      controller.pc);
      fflush(stderr);
      return (0);
    }
  controller.pcstack[controller.pcstack_el++] = (controller.pc + 1) % PROG_SIZE;
#if DEBUG
  fprintf(stderr, "sim: Pushing [%d] onto PC stack\n", controller.pc);
  fflush(stderr);
#endif	
  return (1);
}

int Pop_PC(void)
{
  if (controller.pcstack_el == 0)
    {
      Status_Register.pc_stack_underflo = 1;
      TriggerIRQ();
      controller.execution_halted = 1;
      fprintf(stderr, "kestrel sim: PC stack underflow at %d.\n",
	      controller.pc);
      fflush(stderr);
      return (0);
    }
  controller.pc = controller.pcstack[--controller.pcstack_el];
#if DEBUG
  fprintf(stderr, "sim: Popping [%d] off PC stack\n", controller.pc);
  fflush(stderr);
#endif
  return (1);
}


int Push_Cntr(void)
{
  if (controller.cntrstack_el == 15)
    {
      Status_Register.cntr_stack_overflo = 1;
      TriggerIRQ();
      controller.execution_halted = 1;
      fprintf(stderr, "kestrel sim: counter stack overflow at %d.\n",
	      controller.pc);
      fflush(stderr);
      return (0);
    }
  controller.cntrstack[controller.cntrstack_el++] = instruction.cont_immediate;
#if DEBUG
  fprintf(stderr, "sim: pushed [%d] onto counter stack PC = [%d]\n",
	 controller.cntrstack[0], controller.pc);
#endif
  return (1);
}

int Pop_Cntr(void)
{
  if (controller.cntrstack_el == 0)
    {
      Status_Register.cntr_stack_underflo = 1;
      TriggerIRQ();
      controller.execution_halted = 1;
      fprintf(stderr, "kestrel sim: counter stack underflow at %d.\n",
	      controller.pc);
      fflush(stderr);
      return (0);
    }
  controller.cntrstack_el--;
  return (1);
}

int Dec_Cntr(void)
{
  if (controller.cntrstack_el == 0)
    {
      Status_Register.cntr_stack_underflo = 1;
      TriggerIRQ();
      controller.execution_halted = 1;
      fprintf(stderr, "kestrel sim: counter stack underflow at %d.\n",
	      controller.pc);
      fflush(stderr);
      return (0);
    }

  if (controller.cntrstack[controller.cntrstack_el - 1] == 0) {
    controller.cntrstack[controller.cntrstack_el - 1] = 65535;
  } else {
    controller.cntrstack[controller.cntrstack_el - 1] -= 1;
  }
  if (controller.cntrstack[controller.cntrstack_el - 1] == 65535) {
    Pop_Cntr();
    controller.poped_counter_stack = 1;
  }
  return 1;
}


void Process_PC(void)
{
  int pc_sel;

  if (instruction.push_pc && instruction.pop_pc) {
    fprintf(stderr, "kestrel sim: attempted push and pop of PC stack.\n");
    fflush(stderr);
  } else if (instruction.push_pc) {
    Push_PC();	
  } else if (instruction.pop_pc) {
    Pop_PC();
  }

  /* this is how it's done in the FPGA (OR branch truth with bit 0) */
  pc_sel = instruction.pc_out_sel;

  if( instruction.br_0 && !controller.poped_counter_stack ) {
    pc_sel |= 1;
#if DEBUG
    fprintf(stderr, "sim: counter looping to [%d]\n", controller.pc );
    fflush(stderr);
#endif
  } 

  if( instruction.br_w_or && controller.wor2) {
    pc_sel |= 1;
#if DEBUG
    fprintf(stderr, "sim: WOR cond looping to [%d]\n", controller.pc );
    fflush(stderr);
#endif
  }

  switch(pc_sel) {
  case 0:
    controller.pc = (controller.pc + 1) % PROG_SIZE;
    break;
  case 1:
    controller.pc = instruction.cont_immediate;
    break;
  case 2:
    if (!instruction.pop_pc) {
      fprintf(stderr, "kestrel sim: WARNING: selected PC stack output but no pop!\n");
      fflush(stderr);
    }
    /* the controller.pc field was set by the pop above */
    break;
  case 3:
    /* pc unchanged */
    break;
  default:
    fprintf(stderr, "Error: default case in Process_Jumps() reached\n");
    break;
  }
}

void Process_Cntr(void)
{
  int cur_el;

  if (instruction.push_cnt && 
      (instruction.dec_cnt ||
       (CheckCommandRegMode(CNTR_MODE_DIAG) && instruction.d_pop_counter))) {
      fprintf(stderr, "kestrel sim: attempted push and pop of counter stack.\n");
      fflush(stderr);
      return;
  }
  if (instruction.push_cnt) {
      Push_Cntr();
  } else if (CheckCommandRegMode(CNTR_MODE_DIAG) && instruction.d_pop_counter) {
      Pop_Cntr();
  } else if (instruction.dec_cnt) {
      Dec_Cntr();
  }

  if (instruction.cntr_write == 1) {
    if (controller.cntrstack_el > 0) {
      cur_el = controller.cntrstack[controller.cntrstack_el - 1];
      cur_el = (cur_el & 0xff00) | (controller.Scratch & 0xff);
      controller.cntrstack[controller.cntrstack_el - 1] = cur_el;
    }
  } else if (instruction.cntr_write == 2) {
    if (controller.cntrstack_el > 0) {
      cur_el = controller.cntrstack[controller.cntrstack_el - 1];
      cur_el = (cur_el & 0x00ff) | ((controller.Scratch & 0xff) << 8);
      controller.cntrstack[controller.cntrstack_el - 1] = cur_el;
    }
  }
}

void Process_Cont_Output(void)
{
  unsigned char q_out;

  /* if we did something with the wired-or bit shifter,then
   * that result goes on Qout, otherwise use the scrath register */
  if (instruction.cbs_load || instruction.cbs_sleft) {
    q_out = controller.cbs0 +
            (controller.cbs1 << 1) +
            (controller.cbs2 << 2) +
            (controller.cbs3 << 3) +
            (controller.cbs4 << 4) +
            (controller.cbs5 << 5) +
            (controller.cbs6 << 6) +
            (controller.cbs7 << 7);
  } else {
    q_out = controller.Scratch;
  }
  if ((controller.ArrayDinMask && instruction.data_write) || instruction.fifo_out) {
    /* Program fifo mode acts just like diagnostic mode except data written to
     * the output queue goes into the programmable almost empty/full registers */
    if (controller.state == CNTR_MODE_PROG_FIFO) {
	WriteQueueLevel(MODIFY_LEVEL_QOUT, &QOutAlmostFullLevel,
			&WriteQOutLevelPtr, q_out);
	Status_Register.o_almost_full = IsQOutAlmostFull();
    } else {
      QOutPutData(q_out);
    }
  }
}

void Process_CBS(void)
{
  if ( (instruction.cbs_load) && (instruction.cbs_sleft) )
    {
      fprintf(stderr, "kestrel sim: attempt to load the cbs in serial and in parallel\n");
      fflush(stderr);
    }
 if (instruction.cbs_load)
    {
      Load_CBS(); 
    }
  else if (instruction.cbs_sleft)
    {
      Shift_CBS();
    }
}

void Pre_Process_Scratch(void)
{
}

void Process_Scratch(void)
{
  if (instruction.scr_store) {
    switch(instruction.scr_select) {
    case SCR_MUX_DATA: /*Kestrel left/right output*/
      if (instruction.dest > 31) {
	controller.Scratch = controller.ArrayDataInLeft;
      } else {
	controller.Scratch = controller.ArrayDataInRight;
      }
      break;
    case SCR_MUX_UNUSED :
      controller.Scratch = 0; /* FPGA design currently gives a zero */
      break;
    case SCR_MUX_QIN: /*Q_in*/
      /* In the current controller design, this is not the same
       * QIn of the current instruction, but the QIn of two	
       * instructions later (which may or may not be the same)
       */
      controller.Scratch = controller.qin_poped_data;
      break;
    case SCR_MUX_CBS: /*CBS*/
      controller.Scratch = (controller.cbs0) + (controller.cbs1 * 2) +
	(controller.cbs2 * 4) + (controller.cbs3 * 8) + (controller.cbs4 * 16)
	+ (controller.cbs5 * 32) + (controller.cbs6 * 64) + 
	(controller.cbs7 * 128);
      break;
    default:
      fprintf(stderr, "serial: default case in Load_Scratch() reached\n");
      break; 
    }
  }
}


void ComputeDiagnosticOutput(void)
{
  /* In diagnostic mode, the diagnostic output uses d_out 
   * to control a mux that selects various internal controller
   * values. If not in diagnostic mode, it indicates the internal
   * state of the stacks and breakpoint bit */
  if (CheckCommandRegMode(CNTR_MODE_DIAG)) {
    switch(instruction.d_out)
      {
      case DIAG_MUX_CBS:
	Status_Register.diagnostic_data = 
	  ((controller.cbs0)      + (controller.cbs1 << 1) +
	   (controller.cbs2 << 2) + (controller.cbs3 << 3) + 
	   (controller.cbs4 << 4) + (controller.cbs5 << 5) + 
	   (controller.cbs6 << 6) + (controller.cbs7 << 7));
	break;
      case DIAG_MUX_SCR:
	Status_Register.diagnostic_data = controller.Scratch;
	break;
      case DIAG_MUX_CTR:
	Status_Register.diagnostic_data = controller.cntrstack_el & 0xf;
	break;
      case DIAG_MUX_PCSTACK:
	Status_Register.diagnostic_data = controller.pcstack_el & 0xf;
	break;
      case DIAG_MUX_CTRTOS_LO: 
	if (controller.cntrstack_el == 0) {
	  Status_Register.diagnostic_data = 0;
	} else {
	  Status_Register.diagnostic_data = 
	    controller.cntrstack[controller.cntrstack_el - 1] & 0xff;
	}
	break;
      case DIAG_MUX_CTRTOS_HI:
	if (controller.cntrstack_el == 0) {
	  Status_Register.diagnostic_data = 0;
	} else {
	  Status_Register.diagnostic_data = 
	    (controller.cntrstack[controller.cntrstack_el - 1] >> 8) & 0xff;
	}
	break;
      case DIAG_MUX_PC_LO:
	Status_Register.diagnostic_data = controller.pc & 255;
	break;
      case DIAG_MUX_PC_HI:
	Status_Register.diagnostic_data = (controller.pc >> 8) & 0xff;
	break;
      }
  } else {
    Status_Register.diagnostic_data =
      (Status_Register.cntr_stack_overflo) +
      (Status_Register.cntr_stack_underflo << 1) +
      (Status_Register.pc_stack_overflo << 2) +
      (Status_Register.pc_stack_underflo << 3) +
      (Status_Register.break_bit << 4);
    /* top three bits are currently zero */
  }
}

void Pre_Controller_Execute(void)
{
  Pre_Process_Scratch();

  /* determine data output from controller to array */
  
  /* here Out and In are from the controller's perspective, while
   * data_read and data_write are from the array's perspective 
   */
  if (instruction.data_read) { /* array wants to read data? */
    if (instruction.dest > 31) { /* left => input, right => output */
      /* Where should we take the data from (scratch reg. or QIn ?) */
      if (instruction.in_data_sel) {
	controller.ArrayDataOut = controller.qin_poped_data;
      } else {
	controller.ArrayDataOut = controller.Scratch;
      }
      /* the input is what we wrote */
      controller.ArrayDataInRight = controller.ArrayDataOut;

      /* array will fill in ArrayDataInLeft and ArrayDinMask */
    } else { /* left => output, right => input */
      if (instruction.in_data_sel) {
	controller.ArrayDataOut = controller.qin_poped_data;
      } else {
	controller.ArrayDataOut = controller.Scratch;
      }
      /* the input is what we wrote */
      controller.ArrayDataInLeft = controller.ArrayDataOut;
      
      /* array will fill in ArrayDataInRight and ArrayDinMask */
    }
    controller.ArrayDoutMask = 1;
  } else {
    controller.ArrayDoutMask = 0;
  }

  /* determine immediate field */
  if (instruction.board_imm_mux == 0) {
    controller.ArrayImmediate = controller.Scratch;
  } else {
    controller.ArrayImmediate = instruction.imm;
  }

}

void Post_Controller_Execute(void)
{
#if DEBUG
  int i;

  fflush(stderr);
  printf("instr: %d PC Stack: ", controller.pc);
  for (i = 0; i < 16; i++) {
    printf("%d ", controller.pcstack[i]);
  }
  printf(" Cntr stack: ");
  for (i = 0; i < 16; i++) {
    printf("%d ", controller.cntrstack[i]);
  }
  printf("\n");
  fflush(stdout);
#endif

  /* reset counter stack poped flag */
  controller.poped_counter_stack = 0;

  Process_CBS();
  Process_Scratch();
  Process_Cntr();
  Process_PC();
  Process_Cont_Output();

  ComputeDiagnosticOutput();
}

int Execute_Instruction(int state_was_stop, int state)
{
  controller.state = state;

  if (!Interpret_Inst()) {
    printf("kestrel sim: FATAL ERROR: Interpret_Inst failed.\n");
    exit(99);
  }

#if  DEBUG > 1
  fprintf(stderr, "Executing Instruction @ %d\n", controller.pc);
  PrintInstruction();
#endif

  /* ignore break bit if not running or this is the first instruction
   * executed after going from STOP to RUN */
  if (instruction.break_bit && 
      !state_was_stop &&
      state == CNTR_MODE_RUN) {
    Status_Register.break_bit = 1;
    controller.execution_halted = 1;
    TriggerIRQ();
    return (0);
  }

  /* don't execute the instruction if the input queue is empty or the
   * output queue is full */
  if ((IsQOutFull() || (IsQInEmpty() && Status_Register.valid_next_data == 0))
      && (state != CNTR_MODE_DIAG && state != CNTR_MODE_PROG_FIFO)) {
    controller.execution_halted = 1;
    TriggerIRQ();
    return (0);
  }

  if ((instruction.in_data_sel && instruction.data_read) ||
      instruction.scr_select == SCR_MUX_QIN) {
#if DEBUG
    fprintf(stderr, "kestrel sim: want data from Qin\n\
in_data_sel: %d data_read: %d scr_sel: %d dest: %d\n",
	    instruction.in_data_sel,
	    instruction.data_read,
	    instruction.scr_select,
	    instruction.dest);
    fflush(stderr);
#endif
    QInGetData(&(controller.qin_poped_data));
#if DEBUG
    fprintf(stderr, "Using data from QIn: %d\n", controller.qin_poped_data);
    fflush(stderr);
#endif
  }

  Pre_Controller_Execute();
  Array_Execute();  
#if DEBUG 
  fprintf(stderr, "dinleft: %d dinright: %d dout: %d imm: %d domask: %d dinmask: %d\n",
	  controller.ArrayDataInLeft, controller.ArrayDataInRight,
	  controller.ArrayDataOut, controller.ArrayImmediate,
	  controller.ArrayDoutMask, controller.ArrayDinMask);
  fflush(stderr);
#endif
  Post_Controller_Execute();

  return (1);
}

