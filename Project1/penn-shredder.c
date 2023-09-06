#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

void executeShell(int timeout);

void writeToStdout(char *text);

void alarmHandler(int sig);

// below if the signal handler routing for sigint
void sigintHandler(int sig);

char *getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void trim(char *s);

int main(int argc, char **argv)
{
    registerSignalHandlers();

    int timeout = 0;
    // main needs 2 variable , name of the file and timeout to throw an error.
    if (argc == 2)
    {
        // converts argc 2 into an integer.
        timeout = atoi(argv[1]);
    }
    // if no argss provided, ignore timeout value
    if (timeout < 0)
    {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1)
    {
        executeShell(timeout);
    }

    return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess()
{
    if (kill(childPid, SIGKILL) == -1)
    {
        perror("Error in kill \n");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig)
{
    if (sig == SIGALRM)
    {
        printf("Received SIGNALRM .\n");
    }
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig)
{
    // making sure we are in parent
    if (childPid != 0)
    {
        // killChildProcess();
    }
}

/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers()
{
    // using a signal Api - we are changing the default behaviour of SIGINT which Ctrl C
    // signal(SIGINT, sigintHandler is assigning SIGINT to sigintHandler
    if (signal(SIGINT, sigintHandler) == SIG_ERR)
    {
        perror("Error in signal /n");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.

 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout)
{
    // declare a pointer named command
    char *command;
    // declaring a int variable status
    int status;
    // declaring and initializing array of char
    char minishell[] = "penn-shredder# ";
    writeToStdout(minishell);

    // as soon as below is called all keystrokes are going to be saved on heap
    command = getCommandFromInput();
    if (command != NULL)
    {
        // printf("Command is not NULL %s\n", command);
        childPid = fork();

        // will only throw error - if fork return negative value and is unable to create a process.
        if (childPid < 0)
        {
            perror("Error in creating child process \n");
            free(command);
            exit(EXIT_FAILURE);
        }
        // this "if" is true for child process - anything inside is a child proccess;
        if (childPid == 0)
        {
            // the current code supports simple commands only
            // for example, /bin/ls
            // it does not support /bin/ls -a
            // the execve function accepts the arguments - command such as /bin/ls, it is C-String
            // args: it is array of pointers, each pointer points to array, the
            // last element of args should be NULL
            // envVariables is the array  of environment variables such as
            //{PATH='...', HAME='...'}
            char *const envVariables[] = {NULL};
            char *const args[] = {command, NULL};

            // from man7.org
            // int execve(const char *pathname, char *const argv[], char *const envp[]);
            if (execve(command, args, envVariables) == -1)
            {
                perror("Error in execve");
                free(command);
                exit(EXIT_FAILURE);
            }
            else
            {
                free(command);
            }
        }
        else

        // parent proccess
        {
            do
            {
                if (wait(&status) == -1)
                {
                    perror("Error in child process termination");
                    free(command);
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            free(command);
        }
    }
}

/* Writes particular text to standard output */
void writeToStdout(char *text)
{
    if (write(STDOUT_FILENO, text, strlen(text)) == -1)
    {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char *getCommandFromInput()
{
    // inputBuffer is going to be persisted on heap
    // clear this out once work is done
    char *inputBuffer = (char *)malloc(INPUT_SIZE);

    char command[INPUT_SIZE];
    // read the keystrokes and save it in numByttes
    int numBytes = read(STDIN_FILENO, command, INPUT_SIZE);

    // if control D is presseed
    if (numBytes == 0)
    {
        writeToStdout("Control D pressed, program ending");
        free(inputBuffer);
        exit(1);
    }

    // trim leading and trailing spaces
    int i; // loop control variable
    for (i = 0; i < numBytes; i++)
    {
        // ignore the white space
        // printf("Command[i] is  %c\n", command[i]);
        // to handle a new line in c
        if (command[i] == '\n') // if new line is entered, ignore it and exit the loop
        {
            break;
        }
        inputBuffer[i] = command[i];
    }
    /// C string ends with '\0' character
    inputBuffer[i] = '\0';
    //  remove the trailing spaces and add a null terminator to the end
    trim(inputBuffer);

    // return NULL when you have only entered
    if (strlen(inputBuffer) == 0)
    {
        // printf("EMEPTY COMMAND \n");
        free(inputBuffer);
        return NULL;
    }
    // printf("Command is  %s\n", command);
    // printf("strlen  of command is %lu\n", strlen(command));
    printf("inputBuffer is  %s\n", inputBuffer);

    //   printf("strlen inputbuffer is  %lu\n", strlen(inputBuffer));
    return inputBuffer;
    // return "/bin/ls";
}

void trim(char *s)
{
    int i = strlen(s) - 1;
    // i>0 means you are on the very first character
    while (i > 0)
    {
        if (s[i] == ' ' || s[i] == '\t')
            i--;
        else /* where you see very first from the last non whie space*/
            break;
    }
    s[i + 1] = '\0';
    // at what countt there is a non whitte space
    int count = 0;
    while (s[count] == ' ')
        count++;
    // once you know the 1st non white space
    //- you will start shfting the bits forward.
    int x = 0;
    for (x = 0; s[x + count] != '\0'; x++)
        s[x] = s[count + x];
    s[x] = '\0';
}

// if ctrl D is pressed on parent - quit program if no other processs is running
// if ctrl D is pressed with a valid command - excute  the command and then run control D