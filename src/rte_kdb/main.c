/* $Id: main.c,v 1.26 2001/06/13 22:47:55 ericp Exp $ */
/* 
 * Kestrel Run Time Environment 
 * Copyright (C) 1998 Regents of the University of California.
 * 
 * main.c - global variables and the main fuction that forks
 *	    the simulator or opens a connection to the board server 
 * 	    and either runs the debugger (KDB) or runs the user program.

    12/98 leslie added multiple input files.
 */

#include "globals.h"
#include "format.h"
#include "print.h"
#include "precision.h"
#include "commands.h"
#include "program.h"
#include "interface.h"
#include "breakpoints.h"
#include "kdb.h"
#include "remote.h"
#include "server.h"

/* Global Variables */
char lastMenu;	   
char CurrentMenu;                    /* a char to keep track of the current menu */
char input_buffer[MAX_INPUT_LENGTH]; /* user command input buffer */
char *white_space = " \t\n\r"; /* used with strtok to process user commands */
char print_msg[1024]; /* space to format messages for ErrorPrint and ScreenPrint */

/* pipe handles for communication with the simulator or board */
int write_to_kestrel;
int read_from_kestrel;

/* user setttings */
int have_board = 0;		/* using the board or the simulator? */
int do_not_load_instr = 0;/* assume program was already loaded into instruction memory */
int use_server_func = 1;	/* use server routines to execute the program */
int user_show_irq = 0;		/* display IRQs from the board/simulator */

/* number of processors */
int NumberOfProcs;

InputSpecifierList DataInList;

void addToDataInList( char *s, int input_format, int isRemote)
{
  int i = DataInList.number;

  /*printf("%s, %d %d\n",s, input_format, isRemote); */
  if (i < MAX_NUM_INPUT_FILES ) {
    if (isRemote && input_format != FILE_FORMAT_BINARY) {
      fprintf(stderr, "incompatible options: remote files are always interpreted as binary files.\n");
      exit(99);
    }
    DataInList.a[i].input_filename = s;
    DataInList.a[i].input_format = input_format;
    DataInList.a[i].isRemote = isRemote;
    i++;
    DataInList.number = i;
  } else {
    fprintf(stderr, "You can only give at most %d input files.\n",
             MAX_NUM_INPUT_FILES);
    exit(1);
  }
}

void ErrorPrint(char *func, char *msg)
{
  fprintf(stderr, "kestrel rte: ERROR in %s: %s\n", func, msg);
  fflush(stderr);
}

void ScreenPrint(char *msg)
{
  fprintf(stdout, "kestrel rte: %s", msg);
  fflush(stdout);
}

void DebugPrint(char *msg)
{
  fprintf(stderr, "kestrel rte: %s\n", msg);
  fflush(stderr);
}

char *GetNextArg(char *option, char **argv, 
		      int argc, int *param)
{
  if (*param == argc - 1) {
    fprintf(stderr, "No argument for %s option.\n", option);
    printUsage();
    exit(1);
  }

  (*param) ++;

  return(argv[*param]);
}

void GetFileFormatArg(char *option, int *io_format, char **argv, 
		      int argc, int *param)
{
  if (*param == argc - 1) {
    fprintf(stderr, "No argument for %s option.\n", option);
    printUsage();
    exit(1);
  }

  (*param) ++;

  if (!strcmp("decimal", argv[*param])) {
    *io_format = FILE_FORMAT_DECIMAL;
  } else if (!strcmp("hex", argv[*param])) {
    *io_format = FILE_FORMAT_HEXIDECIMAL;
  } else if (!strcmp("octal", argv[*param])) {
    *io_format = FILE_FORMAT_OCTAL;
  } else if (!strcmp("binary", argv[*param])) {
    *io_format = FILE_FORMAT_BINARY;
  } else {
    fprintf(stderr, "bad format %s for %s option.\n", argv[*param], option);
    printUsage();
    exit(1);
  }
}


int main(int argc, char **argv)
{
  int param; 
  UserProgram *program;
  char sim_write_end[1];  
  char sim_read_end[1];

  /* things set by command line args */
  int run_debugger = 0;		/* envoke KDB */
  char *SourceName = NULL;
  /*  char *DataInName = NULL; */
  char *DataOutName = NULL, *ProcNum = NULL;
  int input_format = FILE_FORMAT_DECIMAL, output_format = FILE_FORMAT_DECIMAL;
  int input_remote = 0;		/* input file is remote (on NT disk) */
  int output_remote = 0;	/* output file is remote (on NT disk) */
  int gave_input_format = 0, gave_output_format = 0;
  int machine_select = 0; /* 2 = marsh, 1 = merlin */

  int curInputFormat = input_format;
  int curInputRemote = 0;
  char *curFileName;
  int somethingWasRemote = 0;

  /* PIPE handles to communicate with the simulator */
  int filedes[2]; /*the file desc array...read end goes to simulator */
  int simfiledes[2]; /* write end of this goes to the simulator. */

  printf("Kestrel Run Time Environment (compiled %s)\n", DATE);
  printf("Copyright (c) 1998 Regents of the University of California\n\n");
  fflush(stdout);
 
  if (argc < 4) {
    printUsage();
    exit(1);
  }

  for (param = 1; param < argc; param++) {
    if (argv[param][0] != '-') {
      if (SourceName == NULL) {
	SourceName = argv[param];
      } else if (DataInList.number == 0) {
	/*if (DataInName == NULL) { */
	addToDataInList(argv[param], input_format, input_remote);
	/*	DataInName = argv[param]; */
      } else if (DataOutName == NULL) {
	DataOutName = argv[param];
      } else if (ProcNum == NULL) {
	if (have_board == 1) {
	  fprintf(stderr, "can't specify processor count with -b\n");
	  printUsage();
	  exit (99);
	} 
	ProcNum = argv[param];
      } else {
	fprintf(stderr, "bad command line argument %s\n", argv[param]);
	printUsage();
	exit (99);
      }
    } else {
      if (!strcmp("-b", argv[param])) {
	have_board = 1;
      } else if (!strcmp("-s", argv[param]) || !strcmp("-sim", argv[param])) {
	have_board = 0;
      } else if (!strcmp("-debug", argv[param])) {
	run_debugger = 1;
      } else if (!strcmp("-machine", argv[param])) {
	param++;
	if (!strcmp("marsh", argv[param])) {
	  machine_select = 2;
	} else if (!strcmp("merlin", argv[param])) {
	  machine_select = 1;
	} else {
	  fprintf(stderr, "bad command line argument -machine %s\n", argv[param]);
	}
      } else if (!strcmp("-noloadi", argv[param])) {
	do_not_load_instr = 1;
      } else if (!strcmp("-slow", argv[param])) {
	use_server_func = 0;
      } else if (!strcmp("-showirq", argv[param])) {
	user_show_irq = 1;
      } else if (!strcmp("-iformat", argv[param])) {
	if (SourceName == NULL) {
	  gave_input_format = 1;
	  GetFileFormatArg("-iformat", &input_format, argv, argc, &param);
	  curInputFormat = input_format;
	} else {
	  GetFileFormatArg("-iformat", &curInputFormat, argv, argc, &param);
	}
      } else if (!strcmp("-oformat", argv[param])) {
	gave_output_format = 1;
	GetFileFormatArg("-oformat", &output_format, argv, argc, &param);
      } else if (!strcmp("-iremote", argv[param])) {
	/* since there is no switch for undoing remote, the default for
multiple files is that everything is NOT remote, so the -iremote is necessary
for every remote file. */
	if (have_board == 0) {
	  fprintf(stderr, "incompatible options, remote and NOT board\n");
	  exit(99);
	}
	if (SourceName == NULL) {
          input_remote = 1;
somethingWasRemote |= input_remote;
	} else {
	  curInputRemote = 1;
somethingWasRemote |= curInputRemote;
          curInputFormat = FILE_FORMAT_BINARY; 
	}
      } else if (!strcmp("-oremote", argv[param])) {
	output_remote = 1;
somethingWasRemote |= output_remote;
	output_format = FILE_FORMAT_BINARY;
      } else if (!strcmp("-in", argv[param])) {
	curFileName = GetNextArg("-in", argv, argc, &param);
	addToDataInList(curFileName, curInputFormat, curInputRemote);
	curInputRemote = 0;
	curInputFormat = input_format;
      }
    }
  }

  if (SourceName == NULL) {
    fprintf(stderr, "must specify executable file name\n");
    printUsage();
    exit (99);
  } else if (DataInList.number == 0) {
    /*((DataInName == NULL) {    */
    fprintf(stderr, "must input data file name\n");
    printUsage();
    exit (99);
  } else if (DataOutName == NULL) {
    fprintf(stderr, "must input data file name\n");
    printUsage();
    exit (99);
  }

  if (have_board == 1 && ProcNum != NULL) {
    fprintf(stderr, "can't specify processor count with -b.\n");
    printUsage();
    exit (99);
  } else if (have_board == 0 && ProcNum == NULL) {
    fprintf(stderr, "must specify processor count for simulation.\n");
    printUsage();
    exit (99);
  } else if ((somethingWasRemote) && (have_board == 0 ||
						 run_debugger == 1 ||
						 use_server_func == 0)) {
    fprintf(stderr, "remote files are incompatible with options -slow, -debug, and -s\n");
    exit(99);
    /*  } else if ((gave_input_format && 
	(input_remote && input_format != FILE_FORMAT_BINARY)) || */
  } else if (
	     (gave_output_format && 
	      (output_remote && output_format != FILE_FORMAT_BINARY))) {
    fprintf(stderr, "incompatible options: remote files are always interpreted as binary files.\n");
    exit(99);
  }

  if (have_board == 0 || run_debugger) {
    use_server_func = 0;
  }

  if (have_board) {
    if (!InitializeSocket(machine_select)) {
      exit (20);
    }
    NumberOfProcs = 512;
  } else {
    /* get the number of processors to simulate from the
     * command line; handle the rest after forking */
    if (!IsNumber(ProcNum)) {
      fprintf(stderr, "bad processor count: %s\n", ProcNum);
      printUsage();
      exit (99);
    }
    NumberOfProcs = atoi(ProcNum);
  
    /* create pipes to talk with the simulator/board */
    pipe(filedes); 
    pipe(simfiledes);

    sim_read_end[0] = (char)filedes[0];
    write_to_kestrel = (char)filedes[1];
    read_from_kestrel = (char)simfiledes[0];
    sim_write_end[0] = (char)simfiledes[1];

    if (fork() == 0) {
      printf("kestrel rte: Starting the Kestrel Serial Simulator.\n");
      fflush(stdout);
      execlp("serial", "serial", ProcNum,
	     "-r", sim_read_end, "-w", sim_write_end, 
	     (char*) 0);
      perror("kestrel rte: execlp serial failed");
      printf("kestrel rte: Could not start the Kesrel Serial Simulator.\n");
      fflush(stdout);
      exit(99);
    }
  } 

  /* catch these signals so we can kill the simulator */
  signal(SIGINT, QuitKdb);
  signal(SIGILL, QuitKdb);
  
  InitializeBoard();

  if (use_server_func) {
    /* run the program from the server (more efficient) */
    RunProgramWithServerFunc(SourceName, 
               DataOutName, output_format, output_remote,
	       &DataInList);

  } else {
    program = InitializeUserProgram(SourceName, DataOutName, output_format,
                                    &DataInList);
    if (program == NULL) {
      QuitKdb();
    }
    /* since we are in the simulator, compute the file size for
       development use */
    program->input_total = computeSizeOfDataInProgram(program);

    if (run_debugger) {
      RunKdb(program);
    } else {
      RunFromCommandLine(program);
    }
  }

  return (0);
}


/* end of file 'main.c' */
