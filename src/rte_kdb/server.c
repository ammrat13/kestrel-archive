/* $Id: server.c,v 1.5 1999/01/09 01:07:19 leslie Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * server.c - run a kestrel program using the complex functions in
 *		the kestrel board server.
 */


#include "globals.h"
#include "program.h"
#include "interface.h"
#include "server.h"
#include "kdb.h"


int ReadDataFromKestrel_Server(void)
{
  unsigned char buf[10];

  while (1) {
    KestrelRead(buf, PIPE_WIDTH);
    if (!strcmp((char *)buf, ERROR_SIGNAL)) {
      HandleServerError();
    } else if (!strcmp((char *)buf, SYNC_SIGNAL)) {
      ErrorPrint("ReadDataFromKestrel_Server", "Synchronization failure.");
      QuitKdb();
    } else {
      break;
    }
  }
  return strtoul((char *)buf, NULL, 16);
}

void TransmitInputData(UserProgram *program)
     /*, char *file_name, int remote) */
{
#define MBS 16000
  static unsigned char  in_buf[10];
  unsigned char buffer[MBS+1];
  int buf_pos, size;
  int i, total, ok;
  int giveAZeroByte = 0;

  /*          printf("xmit in\n");  */
for (i = 0; i < program->numInputData; i++) {
  /* we always re-sync with server. */
  write(write_to_kestrel, Put_in_Pipe(KCMD_SPECIFY_QIN, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);

  KestrelRead(in_buf, 9);
  if (strcmp((char *)in_buf, SYNC_SIGNAL)) {
    ErrorPrint("TransmitInputData", "Synchronization failure.");
    QuitKdb();
  }

  if (program->inputData[i].isRemote) {
    sprintf(print_msg, "remote input file: %s\n", program->inputData[i].input_fileName);
    ScreenPrint(print_msg);
    WriteDataToKestrel(DATA_SOURCE_FILE);
    WriteDataToKestrel(strlen(program->inputData[i].input_fileName) + 1);
    KestrelWrite((unsigned char *)(program->inputData[i].input_fileName), 
                   strlen(program->inputData[i].input_fileName) + 1);
    HandleServerError();
  } else {
        /*        printf("xmita %d\n",i); */
    program->currentInputIndex = i;
    program->input_qinpad = 0;
    size = computeSizeOfDataInCurrentFile(program);
        /*     printf("xmitb %d\n",size); */
    if (size < 0) {
      ErrorPrint("TransmitInputData", "get file size failed\n");
      QuitKdb();
    }
    giveAZeroByte = 0;
    /* ONLY IF THIS IS THE LAST INPUT SOURCE, AND IT IS A
      transmitted file, do we have to add the extra zero byte.  this is
      because david said it had to work this way.... */
    if (i == program->numInputData -1) {
      size += 1;
      giveAZeroByte = 1;
    }
        /*  printf("xmitbr %d\n",size);  */
    WriteDataToKestrel(DATA_SOURCE_TRANSMIT);
    WriteDataToKestrel(size);
      /*  printf("xmitc\n");  */
    total = 0;
    while (1) {
      /* printf("xmitloop\n"); */
      for (buf_pos = 0; buf_pos < MBS; buf_pos++) {
	/* get EXACTLY the number of bytes in the file, or
           get the number plus the last zero byte.*/
	if (giveAZeroByte != 0) {
          ok = GetNextInputData(program, &buffer[buf_pos], giveAZeroByte);
	} else {
          ok = rawGetNextInputDataFromFile(program, &buffer[buf_pos]);
	}
        if (!ok) {
	      /* printf("xmitbreak\n");  */
	  break;
        }
      }
      if (total + buf_pos > size) {
	/* error condition!!!  have to fill pipe with exactly the correct
           amount of data so server doesn't hang.*/
        	/* printf("xmitdERROR %d %d %d\n",total, buf_pos,size); */
	buf_pos = size - total;
      } else {
	total += buf_pos;
      }
        /*  printf("xmitd %d\n",total); */
      KestrelWrite(buffer, buf_pos);
      /* printf("xmite written\n"); */
      if (buf_pos < MBS) break;
    }
      /*printf("xmitf %d\n",total); */
    HandleServerError();
  }
}
    /*printf("xmit out\n"); */
}

void GetServerQoutBuffer(UserProgram *program)
{
  unsigned char *buffer;
  int size, buf_pos;

  write(write_to_kestrel, Put_in_Pipe(KCMD_RETRIEVE_QOUT, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);
  
  size = ReadDataFromKestrel_Server();

  buffer = malloc(size);
  if (buffer == NULL) {
    ErrorPrint("GetServerQoutBuffer", "Out of memory.");
    QuitKdb();
  }

  KestrelRead(buffer, size);

  for (buf_pos = 0; buf_pos < size; buf_pos++) {
    PutDataInOutputBuffer(program, buffer[buf_pos]);
  }
  free(buffer);

  HandleServerError();
}

void TransmitProgram(UserProgram *program)
{
  int instr;
  unsigned char in_buf[10];

  write(write_to_kestrel, Put_in_Pipe(KCMD_SPECIFY_PROGRAM, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);

  KestrelRead(in_buf, 9);
  if (strcmp((char *)in_buf, SYNC_SIGNAL)) {
    ErrorPrint("TransmitProgram", "synchronization failure.");
    QuitKdb();
  }
  
  WriteDataToKestrel(program->code_instr);
  WriteDataToKestrel(program->code_offset);
  WriteDataToKestrel(program->code_instr - 1);

  for (instr = 0; instr < program->code_instr; instr++) {
    KestrelWrite((unsigned char *)program->code_instrptr[instr], 25);
  }

  HandleServerError();
}

void ConfigureOutputData(UserProgram *program, char *file_name, int remote)
{
  unsigned char in_buf[10];

  write(write_to_kestrel, Put_in_Pipe(KCMD_SPECIFY_QOUT, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);

  KestrelRead(in_buf, 9);
  if (strcmp((char *)in_buf, SYNC_SIGNAL)) {
    sprintf(print_msg, "synchronization failure: %s\n", in_buf);
    ErrorPrint("ConfigureOutputData", print_msg);
    QuitKdb();
  }
 
  if (remote) {
    sprintf(print_msg, "remote output file: %s\n", file_name);
    ScreenPrint(print_msg);
    WriteDataToKestrel(DATA_SOURCE_FILE); 
    WriteDataToKestrel(strlen(file_name) + 1);
    KestrelWrite((unsigned char *)file_name, strlen(file_name) + 1);
    HandleServerError();
  } else {
    WriteDataToKestrel(DATA_SOURCE_TRANSMIT); 
    HandleServerError();
  }
}

void HandleServerError(void)
{
  unsigned char in_buf[10];
  int error;

  write(write_to_kestrel, Put_in_Pipe(KCMD_GET_ERROR_CODE, READ), PIPE_WIDTH);
  do {
    KestrelRead(in_buf, 9);
  } while (!strcmp((char *)in_buf, ERROR_SIGNAL));
  error = strtoul((char *)in_buf, NULL, 16);
  switch(error) {
  case 0:	/* no error */
    return;
  case SERVER_ERROR_NOMEM :
    ScreenPrint("Server Error: Out of memory.\n");
    break;
  case SERVER_ERROR_UNKNOWN_DATA_SOURCE :
    ScreenPrint("Server Error: Unknown Data Source type.\n");
    break;
  case SERVER_ERROR_INPUT_EXHAUSTED :
    ScreenPrint("Server Error: Program input data exhausted.\n");
    break;
  case SERVER_ERROR_OPEN_FILE_FAILED :
    ScreenPrint("Server Error: Unable to open file.\n");
    break;
  case SERVER_ERROR_FILE_IO_ERROR :
    ScreenPrint("Server Error: File I/O error.\n");
    break;
  default :
    sprintf(print_msg, "Server Error: Unknown code: %x.\n", error);
    ScreenPrint(print_msg);
    break;
  }

  QuitKdb();
}

char *MakeStatusString(int status)
{
  static char string[1024];

  string[0] = 0;
  if (status & STATUS_REG_COUNTER_OVERFLOW) {
    strcat(string, "<COUNTER STACK OVERFLOW>");
  } 
  if (status & STATUS_REG_COUNTER_UNDERFLOW) {
    strcat(string, "<COUNTER STACK UNDERFLOW>");
  }
  if (status & STATUS_REG_PC_OVERFLOW) {
    strcat(string, "<PC STACK OVERFLOW>");
  }
  if (status & STATUS_REG_PC_UNDERFLOW) {
    strcat(string, "<PC STACK UNDERFLOW>");
  }
  if (status & STATUS_REG_BREAKPOINT) {
    strcat(string, "<BREAKPOINT>");
  }
  return string;
}

/*void RunProgramWithServerFunc(char *SourceName, 
			      char *DataInName, 
			      char *DataOutName,
			      int input_format,
			      int output_format, 
			      int input_remote,
			      int output_remote) */
void RunProgramWithServerFunc(char *SourceName, 
			      char *DataOutName,
			      int output_format, 
			      int output_remote,
                              InputSpecifierList *inputs)
{
  UserProgram *program;
  unsigned char print_buf[10];
  char *size_string;
  int status, addr, qin_used, qout_bytes, milliSeconds, qin_total;
  double data_rate;
  char  *out_file = NULL;

  /*  if (input_remote == 0) {
    in_file = DataInName;
  }
  */
  if (output_remote == 0) {
    out_file = DataOutName;
  }

  program = InitializeUserProgram(SourceName, out_file,
				   output_format, inputs);
  if (program == NULL) {
    QuitKdb();
  }

  TransmitInputData(program);
  ConfigureOutputData(program, DataOutName, output_remote);
  TransmitProgram(program);

  write(write_to_kestrel, Put_in_Pipe(KCMD_RUN_PROGRAM, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);

  ScreenPrint("Remote program execution started...");
  fflush(stdout);

 more_reads:
  KestrelRead(print_buf, 9);
  if (!strcmp((char *)print_buf, ERROR_SIGNAL)) {
    HandleServerError();
  } else if (!strcmp((char *)print_buf, UPDATE_SIGNAL)) {
    printf(".");
    fflush(stdout);
    goto more_reads;
  } else if (strcmp((char *)print_buf, TERMINATE_SIGNAL)) {
    sprintf(print_msg, "Bad return value from server: %s.", print_buf);
    ErrorPrint("RunProgramWithServerFunc", print_msg);
    QuitKdb();
  }
  printf("\n");

  status = ReadDataFromKestrel_Server();
  addr = ReadDataFromKestrel_Server();
  qin_total = ReadDataFromKestrel_Server();
  qin_used = ReadDataFromKestrel_Server();
  qout_bytes = ReadDataFromKestrel_Server();
  milliSeconds = ReadDataFromKestrel_Server();

  sprintf(print_msg, "%s complete at %d on signal: %s\n",
	  program->code_fileName, addr, MakeStatusString(status));
  ScreenPrint(print_msg);

  data_rate = ((double)qin_used+(double)qout_bytes)/((double)milliSeconds/1000.0);
  if (data_rate > 1024 && data_rate < 1048576) {	
    data_rate /= 1024;
    size_string = "K";
  } else if (data_rate >= 1048576) {
    data_rate /= 1048576;
    size_string = "M";
  } else {
    size_string = "";
  }
  sprintf(print_msg, "Execution Time: %d.%d s  (%1.3f %sbytes/second)\n",
	  milliSeconds/1000, milliSeconds % 1000, data_rate, size_string);
  ScreenPrint(print_msg);

  sprintf(print_msg, "%d of %d input bytes used; %d output bytes\n",
	  qin_used, qin_total, qout_bytes);
  ScreenPrint(print_msg);
  
  fflush(stdout);

  if (out_file) {
    GetServerQoutBuffer(program);
    FlushDataOutputBuffer(program);
  }

  write(write_to_kestrel, Put_in_Pipe(KCMD_TERMINATE_PROGRAM, WRITE), PIPE_WIDTH);
  write(write_to_kestrel, Put_in_Pipe(0, 0), PIPE_WIDTH);

  CloseUserProgram(program);
}
