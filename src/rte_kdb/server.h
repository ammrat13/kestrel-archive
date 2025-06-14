
#ifndef _SERVER_H_
#define _SERVER_H_

#define DATA_SOURCE_TRANSMIT	1
#define DATA_SOURCE_FILE	2
#define DATA_SOURCE_SERVER	3

#define SERVER_ERROR_NOMEM 1
#define SERVER_ERROR_UNKNOWN_DATA_SOURCE 2
#define SERVER_ERROR_INPUT_EXHAUSTED 3
#define SERVER_ERROR_OPEN_FILE_FAILED 4
#define SERVER_ERROR_FILE_IO_ERROR 5

/* void TransmitInputData(UserProgram *program, char *file_name, int remote); */
void TransmitInputData(UserProgram *program);
void GetServerQoutBuffer(UserProgram *program);
void TransmitProgram(UserProgram *program);
void ConfigureOutputData(UserProgram *program, char *file_name, int remote);

void RunProgramWithServerFunc(char *SourceName, 
			      char *DataOutName,
			      int output_format, 
			      int output_remote,
                              InputSpecifierList *inputs);
void HandleServerError(void);
char *MakeStatusString(int status);

#endif 
