myShell.c
Written by: Haifaa Abushaaban

NOTE:
    commands are between square braces - [command]

HOW TO RUN:
    1. make sure the profile file called ".CIS3110_profile" is in the same directory as the executable file
    2. $ make
    3. $ ./myShell
        a. export commands MUST be in this format
           [export myPATH=/usr:/usr/local/bin:$myPATH] or
           [export myHOME=/newHome:$myHOME] or
           [export myHISTFILE=new_histfile_name:$myHISTFILE]
           ie, the '=' sign before and the ':' sign after the parameter

IMPROVEMENTS:

    1. Combination:
        Multiple redirection of files (pipe and redirection DOES work, but NOT multiple redirection on the same command)
        a. Example: [sort < ls > file.txt] will NOT work
        b. Example: [sort < numfile.txt | more] WILL work
    2. Space delimiters:
        Spaces for redirection of files (if a command with redirection is present, then the '<' and '>' symbols must be separated by a space)
        a. Example: [sort<numfile.txt] will NOT work
        b. Exmaple: [cat file.txt|more] WILL work
    3. Max input line length is 500 characters

TEST PLAN:

01 Set 1 Functions

    1. exit
        a. Example: [exit]
        b. Expecation: kills child processes still running, terminates
    2. single command
        a. Example: [ls], [pwd]
        b. Expecation: what bash does
    3. multiple arguments
        a. Example: [ls -l], [rm file.txt], 
        b. Expecation: what bash does
    4. executable files
        a. Example: [./helloWorld]
        b. Expecation: what bash does

02 Set 2 Functions

    1. Redirect TO a file
        a. Example: [ls -l > dirFile]
    2. Redirect FROM a file
        a. Example: [sort < numFile.txt]
    3. Multiple redirection
        a. N/A
    4. Pipe
        a. command
            i. Example: [cat largeFile.txt | more]
        b. pipe and redirection:
            i. Example: [sort < numfile.txt | more]
            ii. Expectation: works the same as if typing each command separately (ie, first sort < numfile.txt, then cat numfile.txt | more)
        c. pipe and background process: as long as it is on the end of the second (input) command
            i. Example: [ps aux | sort &]
            ii. Expectation: allows the user to enter in new commands until the sorting has completed
    5. Spacing
        a. No space between a pipe '|' command
            i. Example: [cat file.txt|more]
            ii. Expectation: works the same as if a space is separating the commands 
        b. No space between a redirect '<' '>' command
            i. N/A

03 Set 3 Functions

    1. Environment variables
        a. Call echo with the variables to see if they are displayed
    2. Initialize profile file
        a. Create a profile file called .CIS3110_profile and add commands to it
    3. Built in export command
        a. Run export command with new path, home, and history file arguments, and then
           call echo command to see if the values have changed
    4. Built in history command
        a. Enter commands, run the history command
        b. Run history n to determine the last n commands
        c. Run history -c and then run history to determine if all commands were cleared
    5. Built in cd command
        a. Changing the directory to an invalid directory to determine if an error occurs

FUNCTION DESCRIPTIONS:

(1) MAIN

int main (int argc, char *argv[]);

   the main function will keep prompting the user for new input
   it is also in charge of creating the environment variables
   the main child process pid is kept in the main to kill it once
   the user exits

(2) COMMAND (SETS 1-2)

int childProcess(pid_t childpid, char* line, char* env[4], int* numPaths);

    the childProcess function takes a command and determines which path to use
    the paths include executing the built in functions, execute single command, or pipe command

int executeCommand (char* parameters[MAXPARAM], char* file[4], FILE* fp, char* env[4], int* numPaths);

    the executeCommand function will execute a command with the execve() system call,
    using the environment variables, file name, and path provided
    if redirection is specified it will open a file for redirection prior to executing the command

int pipeFunction (char* line, char* env[4], int* numPaths);

    the pipeFunction funciton will execute a pipe command
    it will split the input and output commands (based on where the '|' symbol is)
    and it will run execute command on either side

(3) BUILT IN (SET 3)

int initializeProfile(char* env[4], int *numPaths);

    this funciton will initialize the profile by reading the contents of the file ".CIS3110_profile"
    line by line and executing each command

void computePath (char* myPATH, char* path, char* file, int numPaths, int location);

    this function will create a path for a command (given in file)
    it is called from the executeCommand function with a file to append to the end
    of the path

void historyFunction(char* parameters[MAXPARAM], int count, char* env[4]);

    this function will execute the 3 history commands history, history -c, and histrory n

void saveHistory(char* history, char* myHISTFILE);

    after evert command is written by the user or run with the profile file, it is saved to the history file
    with this function which appends to the end of the file

void clearHistory(char* myHISTFILE);

    this function will clear the contents of the history file by simply writing nothing to it

void exportFunction(char* command, char* env[4], int* numPaths);

    the export function will change the environment variables
    export commands MUST be in this format
       [export myPATH=/usr:/usr/local/bin:$myPATH] or
       [export myHOME=/newHome:$myHOME] or
       [export myHISTFILE=new_histfile_name:$myHISTFILE]
       ie, the '=' sign before and the ':' sign after the parameter

(4) INPUT & PARSING

int input (char *line, char *file[4], char *parameters[MAXPARAM], int *background, int *status);

    this funciton will parse the input from the users command line,
    as well as determine the type of command (ie, background process, pipe, file redirection, exit, etc

void resetParam(int* isPipe, char* parameters[MAXPARAM], char* file[4]);

    this function will reset the parameters of the commands, file, and the boolean value isPipe
    before every prompt to the user