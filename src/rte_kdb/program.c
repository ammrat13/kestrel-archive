/* $Id: program.c,v 1.14 1999/02/24 23:47:39 dmdahle Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * program.c - routines to initialize and perform large-scale
 *             operations on the UserProgram structure, such as loading
 *	       executable programs and associated data files
 */

#include "globals.h"
#include "pe.h"
#include "program.h"
#include "kdb.h"
#include "breakpoints.h"
#include "interface.h"
#include "code.h"
#include "queues.h"

#define DEBUG 0

/* List of all user programs currently running on the kestrel board. */
UserProgram *user_progs = NULL;

char *my_strdup(char *string)
{
  char *new_string;

  new_string = malloc(strlen(string) + 1);
  if (new_string != NULL) {
    strcpy(new_string, string);
  }
  return (new_string);
}

void DisplayInstruction(UserProgram *program, int addr)
{
  int index, instr, line;
  char *string, *file_name, *instr_copy = NULL;
  char *white_space = " \t\n\r:";
  SourceFile *file;

  if (addr < program->code_offset || addr >= program->code_offset + program->code_instr) {
    sprintf(print_msg, "Invalid instruction address %d: should be in [%d..%d]",
	    addr, program->code_offset, program->code_offset + program->code_instr - 1);
    ErrorPrint("DisplayInstruction", print_msg);
    return;
  } 

  /* correct PC offset from beginning of program */
  addr -= program->code_offset;

  for (index = 0; index < program->code_linecount; index++) {
    if (instr_copy != NULL) {
      free(instr_copy);
      instr_copy = NULL;
    }
    if (program->code_lines[index][0] == ';' &&
	program->code_lines[index][1] == '-') {
      instr_copy = my_strdup(program->code_lines[index]);
      if ((string = strtok(instr_copy, white_space)) == NULL) {
	break;
      }
      if ((string = strtok(NULL, white_space)) == NULL) {
	break;
      }
      instr = strtoul(string, NULL, 16);
      if (instr != addr) {
	continue;
      }
      if ((file_name = strtok(NULL, white_space)) == NULL) {
	break;
      }
  
      if (!strcmp(file_name, "KasmInternal")) {
	printf("KasmInternal: addr %d: %s\n",
	       addr + program->code_offset, 
	       program->code_lines[index+1]);
	break;
      }
    
      if ((file = FindSourceFile(program, file_name)) == NULL) {
	sprintf(print_msg, "could find source file: '%s'", file_name);
	ErrorPrint("DisplayInstruction", print_msg);
	break;
      }

      string = strtok(NULL, white_space);
      line = atoi(string) - 1;

      if (line < 0 || line >= file->line_count) {
	sprintf(print_msg, "line %d for file %s is out of bounds.", line, file_name);
	ErrorPrint("DisplayInstruction", print_msg);
	break;
      }

      printf("%s,%d: addr %d: %s\n", file_name, line, 
	     addr + program->code_offset, 
	     file->lineptr[line]);
      break;
    }
  }

  if (instr_copy != NULL) {
    free(instr_copy);
    instr_copy = NULL;
  }
}

/* 
 * Function   : MoveOutputDataToFile
 * Purpose    : Flush output data from the Kestrel array, and flush data buffered
 *		in memory to the output file.
 * Parameters : program - current program 
 * Returns    : nothing
 * Notes      : 
 */
void MoveOutputDataToFile(UserProgram *program)
{
  FlushQueues(program);
  FlushDataOutputBuffer(program);
}


/* 
 * Function   : End_Program
 * Purpose    : Run when the end-of-program instruction is found,
 *		this routine finishs processing of user input
 *		and output data, and sets the state of the current
 *		program to PROGRAM_STATE_DONE
 * Parameters : program - current program 
 * Returns    : nothing
 * Notes      : 
 */
void End_Program(UserProgram *program)
{
  int unused_bytes, pc;

  PutCommandReg(CNTR_MODE_STOP, 0);

  pc = GetControllerPC();

  MoveOutputDataToFile(program);

  sprintf(print_msg, "program %s completed at %d with %d output bytes.\n",
	  program->code_fileName, pc, program->output_bytesWritten);
  ScreenPrint(print_msg);

  if (program->input_qinpad == 0 ||
      (program->input_qinpad == 1 && program->input_padbytes == 0) ||
      (program->input_qinpad == 1 && 
       program->input_padbytes == program->input_numQinBytes)) {
    /* don't count the extra zero pad included in the input stream */
    if (program->input_gaveLastByte) {
      program->input_numQinBytes --;
    }
    unused_bytes = (program->input_total - program->input_writeBoard) +
      program->input_numQinBytes;
    if (unused_bytes) {
      sprintf(print_msg, "program used only %d of %d input data bytes.\n",
	      program->input_total - unused_bytes, program->input_total);
      ScreenPrint(print_msg);
    }
  } else {
    if (program->input_gaveLastByte) {
      program->input_padbytes++;
    }
    /* FIX THIS FOR MULTIFILES */
    /*    sprintf(print_msg, "the %d bytes in %s were insufficient for program %s.\n",
	    program->input_total, program->input_fileName, program->code_fileName); */
    sprintf(print_msg, "the %d bytes in MULTIFILES were insufficient for program %s.\n",
	    program->input_total, program->code_fileName);
    ScreenPrint(print_msg);
    sprintf(print_msg, "%d data points were appended to the input stream.\n",
	    program->input_padbytes - program->input_numQinBytes);
    ScreenPrint(print_msg);
  }
  program->state = PROGRAM_STATE_DONE;
}

/* 
 * Function   : Run_Program
 * Purpose    : This routine is responsible for starting execution
 *		of a program either from the beginning or from
 *		from a breakpoint (excluding single stepping)
 * Parameters : program - current user program
 *		mode - what state is the program going to run in?
 * Returns    : 0 - program completed, 1 otherwise
 * Notes      : 
 */
int Run_Program(UserProgram *program, int mode)
{
  int pc;

  PutCommandReg(CNTR_MODE_STOP, 0);

  if (program->state == PROGRAM_STATE_INIT) {

    RelocateProgram(program->code_instrptr, 
		    program->code_offset,
		    program->code_instr);

    LoadInstructions(program->code_instrptr, 
		     program->code_instr,
		     program->code_offset);

    SetQInAlmostEmptyTrigger(QIN_ALMOST_EMPTY);
    SetQOutAlmostFullTrigger(QOUT_ALMOST_FULL);

    /* set the PC counter to the correct address */
    SetControllerPC(program->code_offset);

    sprintf(print_msg, "Running %s at %d\n",
	    program->code_fileName, program->code_offset);
    ScreenPrint(print_msg);

    program->state = PROGRAM_STATE_RUNNING;

    PutCommandReg(mode, 0);

  } else if (program->state == PROGRAM_STATE_RUNNING) {
    pc = GetControllerPC();
    sprintf(print_msg, "continuing execution of %s at %d.\n", 
	    program->code_fileName, pc);
    ScreenPrint(print_msg);

    PutCommandReg(mode, 0);

  } else if (program->state == PROGRAM_STATE_DONE) {
    sprintf(print_msg, "program %s already completed execution.\n",
	    program->code_fileName);
    ScreenPrint(print_msg);
    return (0);
  } else {
    return 0;
  }

  return (1);

}  

void disposeInputData(UserProgram *program)
{
  int i;
  for (i = 0; i < program->numInputData; i++) {
    if (program->inputData[i].input_fileName != NULL) { free(program->inputData[i].input_fileName);}
    if (program->inputData[i].input_fileHandle != NULL) { fclose(program->inputData[i].input_fileHandle);}
  }
}

int setUpInputs(UserProgram *program, InputSpecifierList *inputs)
{
  FILE *fp;
  int i;
  int ok = 1;
  /*printf("here %d\n",inputs->number); */
  for (i = 0; i < inputs->number; i++) {
    fp = NULL;
    if (!(inputs->a[i].isRemote)) {
      fp= fopen(inputs->a[i].input_filename, "r");
      if (fp == NULL) {
        printf("rte ERROR: Could not open file: %s\n", inputs->a[i].input_filename);
        ok = 0;
        break;
      }
    }

    program->inputData[i].input_fileName = my_strdup(inputs->a[i].input_filename);
    program->inputData[i].input_format = inputs->a[i].input_format;
    program->inputData[i].isRemote = inputs->a[i].isRemote;
    program->inputData[i].input_fileHandle = fp;
    program->inputData[i].input_position = 0;

    program->numInputData = i+1;
  }
  if (ok == 0) {
    disposeInputData(program);
    return(0);
  }

  program->input_writeBoard = 0;

  program->input_gaveLastByte = 0;
  program->input_gaveLBInstr = 0;

  program->input_recoveredFromQin = NULL;
  program->input_numQinBytes = 0;
  return(1);
}

/* 
 * Function   : InitializeUserProgram
 * Purpose    : loads the .obj file with kestrel instruction, opens the
 *              input and output data files, and loads any debugging
 *              information into a center structure that tracks the
 *              the user program. Initialized format and PE state
 *              information.
 * Parameters : obj_file - name of kestrel object file (from kasm)
 *              input_file - name of input data file
 *              output_file - name of output (destination) data file
 *		input_formast - format of input data file
 *		output_format - format of output data file
 * Returns    : a pointer to a UserProgram structure to be used while
 *              running this program, or NULL on failure.
 * Notes      : 
 */
/*UserProgram *InitializeUserProgram(char *obj_file,
				   char *input_file,
				   char *output_file,
				   int input_format,
				   int output_format) */
UserProgram *InitializeUserProgram(char *obj_file,
				   char *output_file,
				   int output_format,
                                   InputSpecifierList *inputs)
{
  UserProgram *program;
  int initindex;

  program = calloc(1, sizeof(UserProgram));
  if (program == NULL) {
    ErrorPrint("InitializeUserProgram", "ERROR: Out of memory.");
    return NULL;
  }

  /* load user specified files */
  if (!LoadExecutable(program, obj_file)) {
    return NULL;
  }
  if ( ! setUpInputs(program, inputs )) {
    return NULL;
  }
  if (output_file != NULL && !OpenOutputFile(program, output_file, output_format)) {
    return NULL;
  }

  /* initialize PE information structures */
  program->PE = malloc(sizeof(PE_ptr) * (NumberOfProcs + 1));
  if (program->PE == NULL) {
    ErrorPrint("InitializeUserProgram", "ERROR: Out of memory.");
    return NULL;
  }
  for(initindex = 0; initindex <= NumberOfProcs; initindex++ ) {
    program->PE[initindex] = (PE_ptr)PEInfoInit();
  }

  /* initialize register print format information */
  program->reg_info = malloc(sizeof(reg_ptr) * 32);
  if (program->reg_info == NULL) {
    ErrorPrint("InitializeUserProgram", "ERROR: Out of memory.");
    return NULL;
  }
  for(initindex = 0;initindex < 32; initindex++) {
    program->reg_info[initindex] = (reg_ptr)RegInfoInit();
  }

  /* initialize sram print format information */
  program->sram_info = malloc(sizeof(reg_ptr) * 256);
  if (program->sram_info == NULL) {
    ErrorPrint("InitializeUserProgram", "ERROR: Out of memory.");
    return NULL;
  }
  for(initindex = 0;initindex < 256; initindex++) {
    program->sram_info[initindex] = (reg_ptr)RegInfoInit();
  }

  /* initialize controller state storage */
  program->cntr_state = malloc(sizeof(ControllerState));
  if (program->cntr_state == NULL) {
    ErrorPrint("InitializeUserProgram", "ERROR: Out of memory.");
    return NULL;
  }

  /* initialize print format for latches */
  program->latch_format[0] = 'u';
  program->latch_format[1] = 'u';
  program->latch_format[2] = 'u';

  program->next = user_progs;
  user_progs = program;

  program->state = PROGRAM_STATE_INIT;

  program->source_files = NULL;

  return (program);
}


/* 
 * Function   : CloseUserProgram
 * Purpose    : Frees all resources associated with a user program
 *		executed by the run time environment.
 * Parameters : program - UserProgram structure
 * Returns    : nothing
 * Notes      : the memory pointer to by program is free in this call,
 *		making the program pointer invalid.
 */
void CloseUserProgram(UserProgram *program)
{
  int index;
  UserProgram *last_prog, *curr_prog;

  if (program->code_fileName != NULL) {
    free(program->code_fileName);
    if (program->code_breakpoints) {
      free(program->code_breakpoints);
    }
    if (program->code_delinstrptrs) {
      for (index = 0; index < program->code_instr; index++) {
	free(program->code_instrptr[index]);
      }
    }
    free(program->code_instrptr);

    if (program->code_buffer) {
      free(program->code_buffer);
    }

    if (program->code_lines) {
      free(program->code_lines);
    }
  }

  if (program->output_fileHandle != NULL) {
    FlushDataOutputBuffer(program);
    fclose(program->output_fileHandle);
    free(program->output_fileName);
    free(program->output_buffer);
  }

  disposeInputData(program);
  /*  if (program->input_fileHandle != NULL) {
    fclose(program->input_fileHandle);
    free(program->input_fileName);
    free(program->input_buffer);
  }*/
  if (program->input_recoveredFromQin) {
    free(program->input_recoveredFromQin);
  }

  if (program->PE) {
    /* free space for memory formats and values */
    for (index = 0; index < NumberOfProcs; index++) {
      free(program->PE[index]);
    }
    free(program->PE);
  }
  if (program->reg_info) {
    for (index = 0; index < 32; index++) {
      free(program->reg_info[index]);
    }
    free(program->reg_info);
  }
  if (program->sram_info) {
    for (index = 0; index < 256; index++) {
      free(program->sram_info[index]);
    }
    free(program->sram_info);
  }
  if (program->cntr_state) {
    free(program->cntr_state);
  }

  last_prog = NULL;
  curr_prog = user_progs;
  while (curr_prog) {
    if (curr_prog == program) {
      break;
    } else {
      last_prog = curr_prog;
      curr_prog = curr_prog->next;
    }
  }
  if (curr_prog == NULL) {
    ErrorPrint("CloseUserProgram", "Could not find UserProgram on user_prog list.");
  } else {
    if (last_prog == NULL) {
      user_progs = curr_prog->next;
    } else {
      last_prog->next = curr_prog->next;
    }
  }
  
  free(program);
}

void LowerCaseInstructions(UserProgram *program)
{
  int index, up;
  char *instr;

  for (index = 0; index < program->code_instr; index++) {
    instr = program->code_instrptr[index];
    for (up = 0; up < 24; up++) {
      if (instr[up] >= 'A' && instr[up] <= 'F') {
	instr[up] = instr[up] - 'A' + 'a';
      }
    }
  }

}

int IsValidInstruction(char *instr) 
{
  int index;

  if (strlen(instr) != 24) {
    return 0;
  }

  for (index = 0; index < 24; index++) {
    if (!((instr[index] >= '0' && instr[index] <= '9') ||
	  (instr[index] >= 'A' && instr[index] <= 'F') ||
	  (instr[index] >= 'a' && instr[index] <= 'f'))) {
      return 0;
    }
  }

  return (1);
}

/* 
 * Function   : LoadExecutable
 * Purpose    : Reads Kestrel instructions for a user specified
 *              file generated by the Kestrel assembler.
 *              Initializes breakpoint structure.
 * Parameters : program - Information about the current user program.
 *              file_name - name of .obj file to open.
 * Returns    : 1 - success; 0 - failure
 * Notes      : TODO: Relocatable executables
 */
int LoadExecutable(UserProgram *program, char *file_name)
{
  FILE *fp;
  int lines, index, size, read, instr;

  fp = fopen(file_name, "r");
  if (fp == NULL) {
    sprintf(print_msg, "Could not open file: %s\n", file_name);
    ErrorPrint("LoadExecutable", print_msg);
    return (0);
  }
  program->code_fileName = my_strdup(file_name);

  /* get number of bytes in file */
  fseek(fp, 0, SEEK_END);  /* move to end of file */
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);  /* move back to the beginning of the file */

  /* allocate array for pointers to lines */
  program->code_buffer = (char *)malloc(size * sizeof(char));
  if (program->code_buffer == NULL) {
    ErrorPrint("LoadExecutable", "Out of Memory.\n");
    fclose(fp);
    return (0);
  }

  read = fread(program->code_buffer, 1, size, fp);
  if (read != size) {
    sprintf(print_msg, "ERROR: Did not read all bytes from: %s\n", file_name);
    ErrorPrint("LoadExecutable", print_msg);
    fclose(fp);
    return (0);
  }

  fclose(fp);

  /* determine the number of lines in the file */
  lines = 0;
  for (read = 0; read < size; read++) {
    if (program->code_buffer[read] == '\n') {
      lines++;
    }
  }
  if (program->code_buffer[size - 1] != '\n') {
    lines++;
  }
  

  /* allocate memory for line index array */
  program->code_linecount = lines;
  program->code_lines = (char **)malloc(lines * sizeof(char*));
  if (program->code_lines == NULL) {
    ErrorPrint("LoadExecutable", "Out of memory.");
    return (0);
  }

  /* put indices for the beginning of each line into the array */
  program->code_lines[0] = program->code_buffer;
  lines = 1;
  for (read = 0; read < size; read++) {
    if (program->code_buffer[read] == '\n') {
      program->code_buffer[read] = 0;
      if (read != size - 1) {
	program->code_lines[lines++] = &(program->code_buffer[read + 1]);
      }	
    }
  }

  /* calculate number of instructions */
  instr = 0;
  for (index = 0; index < program->code_linecount; index++) {
    if (program->code_lines[index][0] == ';') {
      continue;
    }
    instr++;
  }

  /* allocate memory for line index array */
  program->code_instr = instr;
  program->code_instrptr = (char **)malloc(instr * sizeof(char*));
  if (program->code_instrptr == NULL) {
    ErrorPrint("LoadExecutable", "Out of memory.");
    return (0);
  }

  instr = 0;
  for (index = 0; index < program->code_linecount; index++) {
    if (program->code_lines[index][0] == ';') {
      continue;
    }
    program->code_instrptr[instr++] = program->code_lines[index];
    if (!IsValidInstruction(program->code_instrptr[instr-1])) {
      sprintf(print_msg, "Bad Instruction at line %d: %s\n",
	      index, program->code_lines[index]);
      ErrorPrint("LoadExecutable", print_msg);
      return 0;
    }
  }

  LowerCaseInstructions(program);

  /* TODO: keep a list of used address locations; use that table
   * to track where to put the program. If this is non-zero, we need
   * to relocate the program (branch targets must be adjusted) */
  program->code_offset = 0;

  /* Initialize breakpoint stuff */
  program->code_breakpoints = malloc(lines * sizeof(int));
  if (program->code_breakpoints == NULL) {
    ErrorPrint("LoadExecutable", "Out of memory.\n");
    return (0);
  }

  /* check for existing breakpoints */
  program->code_nextbps = 0;
  for (index = 0; index < program->code_instr; index++) {
    program->code_breakpoints[index] = 0;
    if (SetInstructionBP(program->code_instrptr[index], 'q')) {
      if (!strcmp(program->code_instrptr[index], END_OF_PROGRAM)) {
	sprintf(print_msg, "%s: end of program detected at instruction %d\n",
		file_name, index);
	ScreenPrint(print_msg);
      } else {
	program->code_breakpoints[index] = ++program->code_nextbps;
	sprintf(print_msg, "%s: breakpoint %d detected at instruction %d\n",
		file_name, program->code_breakpoints[index], index);
	ScreenPrint(print_msg);
      }
    }
  }

#if DEBUG
  sprintf(print_msg, "program: %s instr: %d at offset: %d",
	  program->code_fileName, program->code_instr,
	  program->code_offset);
  DebugPrint(print_msg);
#endif

  return (1);
}


/* 
 * Function   : FindSourceFile
 * Purpose    : 
 * Parameters : 
 * Returns    : 
 * Notes      : 
 */
SourceFile *FindSourceFile(UserProgram *program, char *file_name)
{
  FILE *fp;
  int read, size, lines;
  SourceFile *search;

  for (search = program->source_files; search != NULL; search = search->next) {
    if (!strcmp(file_name, search->file_name)) {
      break;
    }
  }

  if (search == NULL) {
    search = malloc(sizeof(SourceFile));
    if (search == NULL) {
      ErrorPrint("FindSourceFile", "Out of memory.");
      return (NULL);
    }

    fp = fopen(file_name, "r");
    if (fp == NULL) {
      sprintf(print_msg, "Could not open source file '%s'", file_name);
      ErrorPrint("FindSourceFile", print_msg);
      return NULL;
    }

    /* get number of bytes in file */
    fseek(fp, 0, SEEK_END);  /* move to end of file */
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);  /* move back to the beginning of the file */

    search->file_name = my_strdup(file_name);
    search->byte_count = size;
    search->buffer = malloc(size);
    if (search->buffer == NULL) {
      ErrorPrint("FindSourceFile", "Out of memory.");
      fclose(fp);
      return NULL;
    }

    read = fread(search->buffer, 1, size, fp);
    if (read != size) {
      sprintf(print_msg, "ERROR: Did not read all bytes from: %s\n", file_name);
      ErrorPrint("FindSourceFile", print_msg);
      fclose(fp);
      return (0);
    }
    fclose(fp);

    /* determine the number of lines in the file */
    lines = 0;
    for (read = 0; read < size; read++) {
      if (search->buffer[read] == '\n') {
	lines++;
      }
    }
    if (search->buffer[size - 1] != '\n') {
      lines++;
    }
  
    search->line_count = lines;
    search->lineptr = malloc(sizeof(char *) * lines);
    if (search->lineptr == NULL) {
      ErrorPrint("FindSourceFile", "Out of memory.");
      return NULL;
    }

    search->lineptr[0] = search->buffer;
    lines = 1;
    for (read = 0; read < size; read++) {
      if (search->buffer[read] == '\n') {
	search->buffer[read] = 0;
	if (read != size - 1) {
	  search->lineptr[lines++] = &(search->buffer[read + 1]);
	}	
      }
    }

    search->next = program->source_files;
    program->source_files = search;
  }

  return search;
}


/* 
 * Function   : OpenInputFile
 * Purpose    : Opens the input data file containing data to be
 * 		feed to the kestrel array during program execution.
 *              No data is read from the file or loaded into the array.
 * Parameters : program - UserProgram structure
 *		file_name - name of disk file to open
 *		input_format - format of input file 
 * Returns    : 1 - success, 0 - failure
 * Notes      : TODO: large data streams. continual loading
 *              of data using interrupts 
 */
#ifdef aaabbbdddd
int OpenInputFile(UserProgram *program, char *file_name, int input_format)
{
  FILE *fp;
  char buffer[MAX_INPUT_LENGTH];
  int data, data_count;
  char *format_string;

  fp= fopen(file_name, "r");
  if (fp == NULL) {
    printf("ERROR: Could not open file: %s\n", file_name);
    return (0);
  }
  program->input_fileName = my_strdup(file_name);
  program->input_fileHandle  = fp;
  program->input_format = input_format;

  /* calculate the number of data points */
  if (program->input_format != FILE_FORMAT_BINARY) {
    program->input_total = 0;
    while (fgets(buffer, MAX_INPUT_LENGTH, fp)) {
      switch (program->input_format) {
      case FILE_FORMAT_DECIMAL :
	data_count = sscanf(buffer, "%d", &data);
	break;
      case FILE_FORMAT_OCTAL :
	data_count = sscanf(buffer, "%o", &data);
	break;
      case FILE_FORMAT_HEXIDECIMAL :
	data_count = sscanf(buffer, "%x", &data);
	break;
      default :
	data_count = 0;
      }
      if (data_count) {
	program->input_total++;
      }
    }
    rewind(fp);
  } else {
    /* determine file length */
    fseek(fp, 0, SEEK_END);
    program->input_total = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  }

  switch(program->input_format) {
  case FILE_FORMAT_DECIMAL :
    format_string = "decimal (ascii)";
    break;
  case FILE_FORMAT_HEXIDECIMAL :
    format_string = "hexidecimal (ascii)";
    break;
  case FILE_FORMAT_OCTAL :
    format_string = "octal (ascii)";
    break;
  case FILE_FORMAT_BINARY :
    format_string = "binary";
    break;
  }
  sprintf(print_msg, "input file %s contains %d bytes in %s\n",
	  program->input_fileName, program->input_total, format_string);
  ScreenPrint(print_msg);

  program->input_position = 0;
  program->input_writeBoard = 0;
  program->input_buffer = malloc(FILE_BUFFER_SIZE);
  if (program->input_buffer == NULL) {
    printf("ERROR: Out of memory.\n");
    return (0);
  }
  program->input_first = 0;
  program->input_last = 0;

  program->input_gaveLastByte = 0;
  program->input_gaveLBInstr = 0;

  program->input_recoveredFromQin = NULL;
  program->input_numQinBytes = 0;

  return (1);
}
#endif

/* 
 * Function   : OpenOutputFile
 * Purpose    : Opens the output file where data generated by the
 * 		user program on the kestrel board is stored.
 * Parameters : program - UserProgram structure for the current program
 *		file_name - name of disk file to open
 * Returns    : 
 * Notes      : TODO: large files, remote files on NT box ... 
 */
int OpenOutputFile(UserProgram *program, char *file_name, 
		   int output_format)
{
  FILE *fp;

  fp = fopen(file_name, "w");
  if (fp == NULL) {
    printf("ERROR: Could not open output file: %s\n", file_name);
    return (0);
  }

  program->output_fileHandle = fp;
  program->output_bytesWritten = 0;
  program->output_fileName = my_strdup(file_name);
  program->output_format = output_format;

  /* open output buffer for data from the PE */
  program->output_buffer = malloc(FILE_BUFFER_SIZE);
  if (program->output_buffer == NULL) {
    printf("ERROR: Out of memory.\n");
    return (0);
  }
  program->output_last = 0;
  program->output_bufsize = FILE_BUFFER_SIZE;

  return (1);
}


/* 
 * Function   : FlushDataOutputBuffer
 * Purpose    : Takes the data output from the kestrel array still
 *		stored in the output buffer in the UserProgram structure
 * 		and writes it to the output file.
 * Parameters : program - UserProgram structure containing the valid
 *			  output file handles and buffer.
 * Returns    : 1 - success, 0 - failure
 * Notes      : 
 */
int FlushDataOutputBuffer(UserProgram *program)
{
  int i;
  char *format_string = "%u\n";

  if (program->output_fileHandle == NULL) {
    sprintf(print_msg, "INTERNAL ERROR: no valid file handle for %s",
	    program->code_fileName);
    ErrorPrint("FlushDataOutputBuffer", print_msg);
    return (0);
  }

  if (program->output_format != FILE_FORMAT_BINARY) {
    switch(program->output_format) {
    case FILE_FORMAT_DECIMAL :
      format_string = "%u\n";
      break;
    case FILE_FORMAT_HEXIDECIMAL :
      format_string = "%x\n";
      break;
    case FILE_FORMAT_OCTAL :
      format_string = "%o\n";
      break;
    }
    for (i = 0; i < program->output_last; i++) {
      fprintf(program->output_fileHandle, format_string,
	      (unsigned int)program->output_buffer[i]);
      program->output_bytesWritten ++;
    }
  } else {
    fwrite(program->output_buffer, 1, program->output_last, 
	   program->output_fileHandle);
    program->output_bytesWritten += program->output_last;
  }
  program->output_last = 0;

  return (1);
}

/* 
 * Function   : PutDataInOutputBuffer
 * Purpose    : Takes the next data byte from the kestrel array and stores
 *		it in the output buffer in the UserProgram structure.
 *		If the buffer is full, it is dumped to disk to make
 *		space.
 * Parameters : program - UserProgram structure containing valid
 *			  ouput file handle and buffer
 *		value - 8-bit value to write to disk
 * Returns    : 1 - success, 0 - failure
 * Notes      : 
 */
int PutDataInOutputBuffer(UserProgram *program, unsigned char value)
{
  if (program->output_last == program->output_bufsize) {
    if (!FlushDataOutputBuffer(program)) {
      return (0);
    }
  }
#if DEBUG
  fprintf(stderr, "kestrel rte: received byte %d from kestrel.\n", value);
  fflush(stderr);
#endif

  program->output_buffer[program->output_last++] = value;
  return (1);
}

int IsInputDataExhausted(UserProgram *program)
{
  if (program->useMemoryInput) {
    return (((program->input_mem_current_index >= program->input_total) &&
            program->input_gaveLastByte) 
         && !program->input_qinpad);
  } else {
    return (((program->currentInputIndex >= program->numInputData) &&
            program->input_gaveLastByte) 
         && !program->input_qinpad);
  }
  /*  return ((program->input_position == program->input_total) &&
            program->input_gaveLastByte) && !program->input_qinpad; */
}

/* 
 * Function   : GetNextInputData
 * Purpose    : Gets the next data byte from the user's input
 *		file. The data is loaded into memory in chunks,
 *		and if there is no new data in memory, then more
 *		is read from disk
 * Parameters : program - UserProgram structure containing valid
 *			  file pointer and bffer
 *		value - pointer to a location to store the
 *		        next character from the data file
 * Returns    : 1 - success, 0 - failure.
 * Notes      : 
 */
/* raw get next input.  only reads data from input files.  if padding is
   going to be done, it must be done by the caller.  this routine does not
   skip to the next input file upon finding EOF.  it just reads the current file.
   returns 0 when reach end of the current file.
   */
int rawGetNextInputDataFromFile(UserProgram *program, unsigned char *value)
{
  int value_read, num_read = 0;
  static char in_buf[1024];
  unsigned char uschar = 0;
  unsigned char uschara[4];
  int ok = 0;
  char *results;

  if (program->currentInputIndex < program->numInputData) {
    if (program->inputData[program->currentInputIndex].input_fileHandle != NULL) {
      if (program->inputData[program->currentInputIndex].input_format != FILE_FORMAT_BINARY) {
	results = fgets(in_buf, 1024, 
               program->inputData[program->currentInputIndex].input_fileHandle);
	if (results != NULL) {
	  /*printf("%s\n", results);*/
	  ok = 1;
          ++(program->inputData[program->currentInputIndex].input_position);
	  num_read = 0;
	  switch (program->inputData[program->currentInputIndex].input_format) {
	  case FILE_FORMAT_DECIMAL :
	    num_read = sscanf(in_buf, "%u", &value_read);
	    break;
	  case FILE_FORMAT_HEXIDECIMAL :
	    num_read = sscanf(in_buf, "%x", &value_read);
	    break;
	  case FILE_FORMAT_OCTAL :
	    num_read = sscanf(in_buf, "%o", &value_read);
	    break;
	  }
	  if (num_read) {
	    uschar = (unsigned char)value_read;
	  } else {
	    ok = 0;
	    uschar = 0;
	    sprintf(print_msg, "%s: bad input at data point at %d: %s",
		    program->inputData[program->currentInputIndex].input_fileName, 
                    program->inputData[program->currentInputIndex].input_position, in_buf);
	    ScreenPrint(print_msg);
	  }
	}
      } else {
	/*printf("%ld\n",program->inputData[program->currentInputIndex].input_buffer);
	  num_read = fread(program->inputData[program->currentInputIndex].input_buffer, 1, 1, */
	num_read = fread(uschara, 1, 1,
			 program->inputData[program->currentInputIndex].input_fileHandle);
        uschar = uschara[0];
	if (num_read == 1) ok = 1;
      }
    }
    if (ok > 0) {
      *value = uschar;
      program->input_writeBoard++;
      return(1);
    }
    *value = 0;
    return(0);
  }
  return(0);
}

int rawGetNextInputDataFromMemory(UserProgram *program, unsigned char *value)
{
  if (program->input_mem_current_index >= program->input_total) {
    *value = 0;
    return(0);
  } else {
    *value = program->input_mem_buffer[program->input_mem_current_index];
    ++(program->input_mem_current_index);
    return(1);
  }
}

int GetNextInputData(UserProgram *program, unsigned char *value, int give_last)
{
  int ok = 0;
  /*
printf("g %d %d %d\n",program->useMemoryInput, program->currentInputIndex,
program->numInputData); */ 
  if (program->useMemoryInput) {
    /* the getprogramstate routine uses memory buffers as input for
       some of it's built in programs. */
    ok = rawGetNextInputDataFromMemory(program, value);
    if (ok > 0) return(1);
  } else {
if (program->currentInputIndex < program->numInputData) {
  while (1) {
    ok = rawGetNextInputDataFromFile(program, value) ;
    /*
printf("  -> %d %d\n",ok, *value);
    */
    if (ok == 0) {
      if (program->currentInputIndex < program->numInputData) {
        /* advance to next input file. */
        ++(program->currentInputIndex);
      } else {
        /* all input from all files has been read. */
        break;
      }
    } else {
      return(1);
    }
  }
}
  }
/* we are here if the data from the input files is all done. */
 
      if (!program->input_qinpad) {
	if (!program->input_gaveLastByte && give_last) {
	  *value = 0;
	  program->input_gaveLastByte = 1;
	  program->input_gaveLBInstr = 0;
	  return (1);
	}
	return (0);
      } else {
	*value = 0;
	program->input_padbytes ++;
	return (1);
      }
      return (0);
}

#ifdef aaabbbbdddd
int GetNextInputData(UserProgram *program, unsigned char *value, int give_last)
{
  int value_read, num_read;
  static char in_buf[1024];

  if (program->input_first == program->input_last) {
    program->input_first = 0;
    program->input_last = 0;

    if (program->input_position < program->input_total &&
	program->input_fileHandle != NULL) {
      if (program->input_format != FILE_FORMAT_BINARY) {
	while (program->input_last < FILE_BUFFER_SIZE &&
	       program->input_position < program->input_total &&
	       !feof(program->input_fileHandle) &&
	       fgets(in_buf, 1024, program->input_fileHandle)) {
	  switch (program->input_format) {
	  case FILE_FORMAT_DECIMAL :
	    num_read = sscanf(in_buf, "%u", &value_read);
	    break;
	  case FILE_FORMAT_HEXIDECIMAL :
	    num_read = sscanf(in_buf, "%x", &value_read);
	    break;
	  case FILE_FORMAT_OCTAL :
	    num_read = sscanf(in_buf, "%o", &value_read);
	    break;
	  }
	  if (num_read) {
	    program->input_buffer[program->input_last] = (unsigned char)value_read;
	    program->input_last++;
	    program->input_position++;
	  } else {
	    sprintf(print_msg, "%s: bad input at data point at %d: %s",
		    program->input_fileName, program->input_position, in_buf);
	    ScreenPrint(print_msg);
	  }
	}
      } else {
	num_read = fread(program->input_buffer, 1, FILE_BUFFER_SIZE,
			 program->input_fileHandle);
	program->input_last += num_read;
	program->input_position += num_read;
      }
    } else {
      if (!program->input_qinpad) {
	if (!program->input_gaveLastByte && give_last) {
	  *value = 0;
	  program->input_gaveLastByte = 1;
	  program->input_gaveLBInstr = 0;
	  return (1);
	}
	return (0);
      } else {
	*value = 0;
	program->input_padbytes ++;
	return (1);
      }
    }
  }

  *value = program->input_buffer[program->input_first++];
  program->input_writeBoard++;
  return (1);
}
#endif

/* reads the rest of the file from the current location, then rewinds file
   to the begining.  returns number of bytes read. returns -1 if an error*/
int computeSizeOfDataInCurrentFile(UserProgram *program)
{
  FILE *fp;
  int total = 0;
  char buffer[MAX_INPUT_LENGTH];

  if (program->currentInputIndex >= program->numInputData) {
    return(-1);
  }

  fp = program->inputData[program->currentInputIndex].input_fileHandle;
  if (fp == NULL) {
    return (-1);
  }

  /* calculate the number of data points */
  if (program->inputData[program->currentInputIndex].input_format != FILE_FORMAT_BINARY) {
    while (fgets(buffer, MAX_INPUT_LENGTH, fp)) {
      total++;
    }
    rewind(fp);
  } else {
    /* determine file length */
    fseek(fp, 0, SEEK_END);
    total = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  }
  return (total);
}

/* ONLY FOR USE IN SIMULATOR!!!! does not understand remote files,
you shouldn't be trying to compute the number of bytes anyway when
using the kestrel board.
ONLY FOR USE at the beginning of a program run, since it changes
the file positions*/
int computeSizeOfDataInProgram(UserProgram *program)
{
  int i, num;
  int size = 0;
  for (i = 0; i < program->numInputData; i++) {
    program->currentInputIndex = i;
    num = computeSizeOfDataInCurrentFile(program);
    program->inputData[i].input_total = num;
    size += num;
  }
  program->currentInputIndex = 0;
  return(size);
}

/* end of file 'program.c' */
