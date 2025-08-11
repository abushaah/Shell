#include <string.h>
#include <stdio.h>       /* Input/Output */
#include <stdlib.h>      /* General Utilities */
#include <unistd.h>      /* Symbolic Constants */
#include <sys/types.h>   /* Primitive System Data Types */
#include <sys/wait.h>    /* Wait for Process Termination */
#include <errno.h>       /* Errors */
#include <signal.h>      /* Signals */
#include "myShell.h"
/*
 *      Author: Haifaa
 *      January 2022 - April 2022
 *      Summary: implementing a UNIX shell program
 *      Compile and Run:
 *         make
 *         ./myShell
 */

/* the main function will keep prompting the user for new input
   it is also in charge of creating the environment variables
   the main child process pid is kept in the main to kill it once
   the user exits
*/
int main (int argc, char *argv[]){

    pid_t childpid;
    int status = 0;
    char directory[MAXPARAM];

    char *line = (char*)(malloc(MAXPARAM));
    if (line == NULL){
        fprintf(stderr, "malloc failed\n");
        return 0;
    }

    // create environment variables
    char* myHOME = (char*)(malloc(MAXPARAM));
    if (myHOME == NULL){
        fprintf(stderr, "malloc failed\n");
        return 0;
    }
    strcpy(myHOME, "myHOME=");
    strcat(myHOME, getenv("HOME"));

    char* myPATH = (char*)(malloc(MAXPARAM));
    if (myPATH == NULL){
        fprintf(stderr, "malloc failed\n");
        return 0;
    }
    strcpy(myPATH, "myPATH=/bin:/usr/bin");

    char* myHISTFILE = (char*)(malloc(MAXPARAM));
    if (myHISTFILE == NULL){
        fprintf(stderr, "malloc failed\n");
        return 0;
    }
    strcpy(myHISTFILE, "myHISTFILE=~/.CIS3110_history");

    char *env[4] =
    {
        myHOME,
        myPATH,
        myHISTFILE,
        0
    };

    int numPaths = 2; // export command changes this when a different path is set

    initializeProfile(env, &numPaths);

    while (1){

        printf("%s> ", getcwd(directory, MAXPARAM));
        fgets(line, MAXPARAM, stdin);
        line[strlen(line) - 1] = '\0'; // remove trailing newline

        if (strncmp(line, "exit", 4) == 0){
            break;
        }

        status = childProcess(childpid, line, env, &numPaths);

        if (status == SIGQUIT || status == FAIL){
            break;
        }
    }

    free(line);
    free(myPATH);
    free(myHOME);
    free(myHISTFILE);

    if (!WIFEXITED(status)){
        kill(childpid, 0);
    }

    exit(EXIT_SUCCESS);
    return 0;

}

// this function will execute the commands from the profile
int initializeProfile(char* env[4], int *numPaths){

    pid_t childpid;
    int status = 0;

    FILE* file = fopen(".CIS3110_profile", "r");
    if (file == NULL){
        fprintf(stderr, "-myShell: ~/.CIS3110_profile: No such file or directory\n");
        return SUCCESS;
    }

    char line[MAXPARAM];
    int i = 0;
    while (fgets (line, MAXPARAM, file) != NULL){

        line[strlen(line) - 1] = '\0'; // remove trailing newline
        status = childProcess(childpid, line, env, numPaths);
        if (status == SIGQUIT || status == FAIL){
            fclose(file);
            return status;
        }
        ++i;
    }

    fclose(file);
    return SUCCESS;

}

// this is the function that takes a command and determines which path to use
// such as built in functions, execute single command, or pipe command
int childProcess(pid_t childpid, char* line, char* env[4], int* numPaths){

    int status = 0; // child exit status

    char* parameters[MAXPARAM];
    int count = 0;

    char* file[4] = {NULL, NULL, NULL, NULL}; // [0] = redirect to, [1] = redirect from, [2] = filename for redirect to, [3] = filename for redirect from
    FILE* fp = NULL;

    int isPipe = 0; // pipe command is false
    int background = 0; // background command is false

    resetParam(&isPipe, parameters, file); // call a function to reset parameters
    saveHistory(line, env[2]); // save the command in the history file

    // determine if the command is a pipe, without altering original components
    char lineCPY[MAXPARAM];
    strcpy(lineCPY, line);
    char *pipe = strtok(lineCPY, "|");
    pipe = strtok(NULL, "|");
    if (pipe != NULL){
        int valid = pipeFunction(line, env, numPaths);
        if (valid == FAIL){
            return FAIL;
        }
        return status;
    }

    count = input (line, file, parameters, &background, &status); // identify the input

    if (status == SIGQUIT){
        return status;
    }

    // environment variables
    if (strcmp(parameters[0], "echo") == 0){
        if (strcmp(parameters[1], "$myHOME") == 0){
            printf("%s\n", env[0]);
            return status;
        }
        else if (strcmp(parameters[1], "$myPATH") == 0){
            printf("%s\n", env[1]);
            return status;
        }
        else if (strcmp(parameters[1], "$myHISTFILE") == 0){
            printf("%s\n", env[2]);
            return status;
        }
    }

    if (strcmp(parameters[0], "export") == 0){
        exportFunction(parameters[1], env, numPaths);
        return status;
    }

    // cd built in function
    if (strcmp(parameters[0], "cd") == 0){
        int valid = chdir(parameters[1]);
        if (valid == -1){
            fprintf(stderr, "-myShell: cd: %s: No such file or directory\n", parameters[1]);
        }
        return status;
    }

    // history built in function
    if (strcmp(parameters[0], "history") == 0){
        if (count >= 3){
            fprintf(stderr, "-myShell: command not found\n");
        }
        historyFunction(parameters, count, env);
        return status;
    }

    // function from the exec() family or an executable
    childpid = fork();

    if (childpid >= 0){
        if (childpid == 0){
            status = executeCommand(parameters, file, fp, env, numPaths);

            if (fp != NULL) fclose(fp); // if there are files open after the command, close the files
            exit(status); // when it finishes the command, exits
        }
        else{ // parent process
            if (background == 0){
                waitpid(childpid, &status, 0); // waits for this specific child to exit
            }
            else{ // background == 1
                waitpid(childpid, &status, WNOHANG); // child process executes in the background
            }
        }
    }
    else{ // -1 means fork failed
        perror("fork");
        free(line);
        exit(EXIT_FAILURE);
    }

    return status;
}

// the input function will parse the input that is given from the command line
int input (char *line, char *file[4], char *parameters[MAXPARAM], int *background, int *status){

    char* token;
    int count = 0; // counts arguments
    int redirectTo = 0;
    int redirectFrom = 0;
    int piped = 0;
    int added[2] = {0, 0};

    if (line[0] == '\n' || line[0] == '\0' || (strcmp(line, "exit") == 0)){
        *status = SIGQUIT;
        return SUCCESS;
    }

    token = strtok(line, DELIM);
    parameters[count] = token;
    count++;

    while (token != NULL){

        token = strtok(NULL, DELIM);
        if (token != NULL){
            if (strcmp(token, ">") == 0){ // redirect to
                file[0] = token;
                redirectTo = 1;
            }
            else if (strcmp(token, "<") == 0){ // redirect from
                file[1] = token;
                redirectFrom = 1;
            }
            else{
                if (redirectTo == 1 && added[0] == 0){ // previous value called for a redirect, current value is file name
                    file[2] = token;
                    added[0] = 1;
                }
                else if (redirectFrom == 1 && added[1] == 0){ // previous value called for a redirect, current value is file name
                    file[3] = token;
                    added[1] = 1;
                }
                else{
                    parameters[count] = token;
                    count++;
                }
            }
        }
    }

    if (count >= 1){ // command is entered
        if (strcmp(parameters[count - 1], "&") == 0){
            parameters[count - 1] = NULL; // remove &
            *background = 1; // set background to true
        }
        else{
            *background = 0;
        }
    }

    if (*background == 0) parameters[count] = NULL;
    return count;

}

int executeCommand (char* parameters[MAXPARAM], char* file[4], FILE* fp, char* env[4], int* numPaths){ // calls execv to execute the function

    int status = 0;

    // an executable file
    if (parameters[0][0] == '.' && parameters[0][1] == '/'){
        status = execv(parameters[0], parameters);
        if (status == -1){
            fprintf(stderr, "-myShell: command not found\n");
        }
        return status;
    }

    if ((file[2] != NULL) && (file[0] != NULL) && (strcmp(file[0], ">") == 0)){ // redirect TO a file
        if (fp == NULL) fp = freopen(file[2], "w+", stdout);
    }
    if ((file[3] != NULL) && (file[1] != NULL) && (strcmp(file[1], "<") == 0)){ // redirect FROM a file
        if (fp == NULL) fp = freopen(file[3], "r", stdin);
        if (fp == NULL){
            fprintf(stderr, "-myShell: %s: No such file or directory\n", file[3]);
            exit(status);
        }
     }

    char *path = (char*)(malloc(MAXPARAM));
    if (path == NULL){
        fprintf(stderr, "malloc failed\n");
        return status;
    }

    int i = 1;
    while (i <= *numPaths){
        computePath (env[1], path, parameters[0], *numPaths, i); // look in first location
        status = execve(path, parameters, env); // will return -1 when command is not found in the path
        if (status == -1){ // since we have returned, the command is not found
            ++i;
            continue;
        }
    }

    if (i > *numPaths){ // still couldnt find command
        fprintf(stderr, "-myShell: command not found\n");
    }

    free(path);
    return status;
}

void computePath (char* myPATH, char* path, char* file, int numPaths, int location){

    char* pathCPY = malloc(strlen(myPATH) + 1);
    if (pathCPY == NULL){
        fprintf(stderr, "malloc failed\n");
        return;
    }
    strcpy(pathCPY, myPATH);

    char* pathToken = strtok(pathCPY, "=");
    pathToken = strtok(NULL, ":");

    int i = 1;
    while (pathToken != NULL && i <= numPaths){
        if (i == location){
            strcpy(path, pathToken);
            strcat(path, "/");
            strcat(path, file);
            break;
        }
        else{
            pathToken = strtok(NULL, ":");
            ++i;
        }
    }

    free(pathCPY);
}

void exportFunction(char* command, char* env[4], int *numPaths){

    if (strncmp(command, "myHOME", 6) == 0){
        char* homeCPY = malloc(strlen(command) + 1);
        if (homeCPY == NULL){
            fprintf(stderr, "malloc failed\n");
            return;
        }
        strcpy(homeCPY, command);

        char* homeToken = strtok(homeCPY, "$");
        homeToken[strlen(homeToken) - 1] = '\0'; // remove trailing ':'
        strcpy(env[0], homeToken); // updated the home directory environment variable
        free(homeCPY);
    }

    if (strncmp(command, "myPATH", 6) == 0){

        char* pathCPY = malloc(strlen(command) + 1);
        if (pathCPY == NULL){
            fprintf(stderr, "malloc failed\n");
            return;
        }
        strcpy(pathCPY, command);

        char* pathToken = strtok(pathCPY, "$");
        pathToken[strlen(pathToken) - 1] = '\0'; // remove trailing ':'
        strcpy(env[1], pathToken); // updated the path environment variable
        free(pathCPY);

        pathCPY = malloc(strlen(env[1]) + 1);
        if (pathCPY == NULL){
            fprintf(stderr, "malloc failed\n");
            return;
        }
        strcpy(pathCPY, env[1]);

        pathToken = strtok(pathCPY, "=");
        pathToken = strtok(NULL, ":");
        int total = 0;
        while (pathToken != NULL){
            pathToken = strtok(NULL, ":");
            ++total;
        }
        *numPaths = total; // updated the num paths variable
        free(pathCPY);
    }

    if (strncmp(command, "myHISTFILE", 6) == 0){
        char* histCPY = malloc(strlen(command) + 1);
        if (histCPY == NULL){
            fprintf(stderr, "malloc failed\n");
            return;
        }
        strcpy(histCPY, command);

        char* histToken = strtok(histCPY, "$");
        histToken[strlen(histToken) - 1] = '\0'; // remove trailing ':'
        strcpy(env[2], histToken); // updated the histfile environment variable
        free(histCPY);
    }

}

void historyFunction(char *parameters[MAXPARAM], int count, char* env[4]){

    char* fileCPY = malloc(strlen(env[2]) + 1); // make a copy since strtok is destructive
    if (fileCPY == NULL){
        fprintf(stderr, "malloc failed\n");
        return;
    }
    strcpy(fileCPY, env[2]);

    char* filename = strtok(fileCPY, "/");
    filename = strtok(NULL, "/");
    if (filename == NULL){
        fprintf(stderr, "-myShell: ~/%s: No such file or directory\n", env[2]);
        free(fileCPY);
        return;
    }

    if ((count == 2) && (strcmp(parameters[1], "-c") == 0)){
        clearHistory(filename);
        free(fileCPY);
        return;
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL){
        fprintf(stderr, "-myShell: ~/%s: No such file or directory\n", env[2]);
        free(fileCPY);
        return;
    }

    char line[MAXPARAM];
    int total = 1;
    while (fgets (line, MAXPARAM, file) != NULL){
        if (count == 1){ // history command
            printf(" %d  %s", total, line);
        }
        ++total;
    }

    if (count == 1){
        fclose(file);
        free(fileCPY);
        return;
    }

    // count == 2
    fseek(file, 0, SEEK_SET);
    int n = atoi(parameters[1]);
    int i = 1;
    int start = total - n;
    while (fgets (line, MAXPARAM, file) != NULL){
        if (i >= start){
            printf(" %d  %s", i, line);
        }
        ++i;
    }

    free(fileCPY);
    fclose(file);
}

// this function clear the history file
void clearHistory(char* myHISTFILE){

    if (myHISTFILE == NULL) return;

    FILE* file = fopen(myHISTFILE, "w");
    if (file == NULL){
        fprintf(stderr, "-myShell: ~/%s: No such file or directory\n", myHISTFILE);
        return;
    }

    fclose(file);
}

// this function will save the command to the history file
void saveHistory(char* history, char* myHISTFILE){

    if (history == NULL || myHISTFILE == NULL) return;

    char* fileCPY = malloc(strlen(myHISTFILE) + 1); // make a copy since strtok is destructive
    if (fileCPY == NULL){
        fprintf(stderr, "malloc failed\n");
        return;
    }
    strcpy(fileCPY, myHISTFILE);

    char* filename = strtok(fileCPY, "/");
    filename = strtok(NULL, "/");
    if (filename == NULL){
        fprintf(stderr, "-myShell: ~/%s: No such file or directory\n", myHISTFILE);
        free(fileCPY);
        return;
    }

    FILE* file = fopen(filename, "a");
    if (file == NULL){
        fprintf(stderr, "-myShell: ~/%s: No such file or directory\n", myHISTFILE);
        free(fileCPY);
        return;
    }

    fprintf(file, "%s\n", history);

    free(fileCPY);
    fclose(file);
}

// the pipe function will be used whne the user enters a pipe command
int pipeFunction (char* line, char* env[4], int* numPaths){

    pid_t childpid; // first child process will run the input command
    pid_t childpid2; // second child process will run the input command
    int statusO = 0;
    int statusI = 0;
    int background = 0; // this variable is only used on the input command, since the '&' sign is always at the end of the command

    char *pipeI[MAXPARAM]; // for input
    char *pipeO[MAXPARAM]; // for output
    int fd[2]; // file descriptors for the pipe

    // file redirection for the commands input and output
    char *fileO[4] = {NULL, NULL, NULL, NULL}; // [0] = redirect to, [1] = redirect from, [2] = filename for redirect to, [3] = filename for redirect from
    FILE* fpO = NULL;
    char *fileI[4] = {NULL, NULL, NULL, NULL};
    FILE* fpI = NULL;

    char* line2 = malloc(strlen(line) + 1); // must create a copy since we will be destroying the original string for delimiters
    if (line2 == NULL){
        fprintf(stderr, "malloc failed\n");
        return FAIL;
    }
    strcpy(line2, line);

    char* outputComm = strtok(line, "|"); // first command is the output
    int countO = input (outputComm, fileO, pipeO, &background, &statusO); // identify the output command given

    char* inputComm = strtok(line2, "|"); // first command is the output
    inputComm = strtok(NULL, "\0"); // first command is the output
    int countI = input (inputComm, fileI, pipeI, &background, &statusO); // identify the input

    if (pipe(fd) == -1){ // create a pipe in parent
        perror("pipe");
        return FAIL;
    }

    childpid = fork();
    if (childpid < 0){
        perror("fork");
        return FAIL;
    }

    if (childpid == 0){
        dup2(fd[1], STDOUT_FILENO); // using unistd library to create a copy of the open file descriptors (which is created by the pipe call above)
        close(fd[0]); // close read end of pipe
        close(fd[1]); // close write end of the pipe
        statusO = executeCommand (pipeO, fileO, fpO, env, numPaths);
        exit(statusO);
    }

    childpid2 = fork();
    if (childpid2 < 0){
        perror("fork");
        return FAIL;
    }

    if (childpid2 == 0){
        dup2(fd[0], STDIN_FILENO); // open the read end of the pipe
        close(fd[0]);
        close(fd[1]);
        statusI = executeCommand (pipeI, fileI, fpI, env, numPaths);
        exit(statusI);
    }

    close(fd[0]); // close read end
    close(fd[1]); // close write end

    waitpid(childpid, &statusO, 0); // waits for first child (output)

    if (background == 0){
        waitpid(childpid2, &statusI, 0); // waits for second child
    }
    else{ // background == 1
        waitpid(childpid2, &statusI, WNOHANG); // child process executes in the background
    }

    if (WIFEXITED(statusI)){ // if the child process has exited, we must free memory
        free(line2);
        if (fpI != NULL) fclose(fpI); // if there are files open after the command, close the files
        if (fpO != NULL) fclose(fpO); // if there are files open after the command, close the files
    }

    return SUCCESS;

}

// this function reset the parameters before every command
void resetParam(int* isPipe, char* parameters[MAXPARAM], char* file[4]){

    isPipe = 0; // reset pipe variable
    memset(parameters, 0, MAXPARAM * sizeof(parameters[0])); // reset parameters list
    memset(file, 0, MAXPARAM * sizeof(file[0])); // reset file list

}
