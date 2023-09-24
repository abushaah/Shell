#ifndef __ABUSHAAH__A1H__
#define __ABUSHAAH__A1H__

#define DELIM " "
#define MAXPARAM 500
#define IN_END 0
#define OUT_END 1

#define SUCCESS 1
#define FAIL -1
#define SIGQUIT 3

// command functions (sets 1 - 2)
int childProcess(pid_t childpid, char* line, char* env[4], int* numPaths);
int executeCommand (char* parameters[MAXPARAM], char* file[4], FILE* fp, char* env[4], int* numPaths);
int pipeFunction (char* line, char* env[4], int* numPaths);

// environment variable functions (set 3)
int initializeProfile(char* env[4], int *numPaths);

void computePath (char* myPATH, char* path, char* file, int numPaths, int location);

void historyFunction(char* parameters[MAXPARAM], int count, char* env[4]);
void saveHistory(char* history, char* myHISTFILE);
void clearHistory(char* myHISTFILE);

void exportFunction(char* command, char* env[4], int* numPaths);

// input parsing functions
int input (char *line, char *file[4], char *parameters[MAXPARAM], int *background, int *status);
void resetParam(int* isPipe, char* parameters[MAXPARAM], char* file[4]);

#endif
