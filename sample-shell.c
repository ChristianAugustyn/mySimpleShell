/**
 * Simple shell interface starter kit program.
 * Operating System C   oncepts
 * Mini Project1
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#define MAX_LINE 80 /* 80 chars per line, per command */

void runProcess(char *args[], pid_t pid, char *history[], int redirect_out, int redirect_in, int out, int in, int amp);
int lengthOfArgs(char *args[]);
int indexOf(char *args[], char *delim);
void clearArgs(char *args[], int start);
void copyArray(char *args[], char *args2[], int index);
void parseArg(char *args[], char line[]);

int main(void)
{
    //array of strings
    char *args[MAX_LINE / 2 + 1] = {'\0'}; /* command line (of 80) has max of 40 arguments */
    char line[MAX_LINE / 2 + 1];
    int should_run = 1;
    char *history[MAX_LINE / 2 + 1] = {'\0'};
    char cwd[MAX_LINE];
    int amp;

    while (should_run)
    {
        amp = 0;
        printf("mysh:~$ ");
        fflush(stdout);
        fgets(line, sizeof(line), stdin);
        pid_t pid;

        parseArg(args, line);
        if(strcmp(args[0], "!!") == 0)
        {
            int it = 0;
            while(history[it] != NULL)
            {
                args[it] = history[it];
                it++;
            }
        } 
        else 
        {
            char *copy = malloc(strlen(line) + 1);
            strcpy(copy, line);
            parseArg(history, copy);
        }

        if (strcmp(args[lengthOfArgs(args) - 1], "&") == 0)
        {
            amp = 1;
            args[lengthOfArgs(args) - 1] = '\0';
        }

        if (strcmp(args[0], "exit") == 0)
        {
            should_run = 0;
        }
        else if (indexOf(args, "|") >= 1 && lengthOfArgs(args) >= 3) //pipe
        {
            runProcess(args, pid, history, 1, 1, 0, 0, amp);
        }
        else if (strcmp(args[0], "cd") == 0 && args[1] != NULL)
        {
            if (chdir(args[1]) == 0)
            {
                printf("%s ", getcwd(cwd, MAX_LINE));
            }
            else
            {
                printf("no such file or directory: %s\n", args[1]);
            }
        }
        else if (lengthOfArgs(args) >= 3 && indexOf(args, ">") > 0)
        {
            int file = creat(args[indexOf(args, ">") + 1], 0664);
            clearArgs(args, indexOf(args, ">"));
            runProcess(args, pid, history, 1, 0, file, 0, amp);
            close(file);
        }
        else if (lengthOfArgs(args) >= 3 && indexOf(args, "<") > 0)
        {
            int file = open(args[indexOf(args, "<") + 1], O_RDONLY);
            clearArgs(args, indexOf(args, "<"));
            runProcess(args, pid, history, 0, 1, 0, file, amp);
            close(file);
        }
        else
        {
            runProcess(args, pid, history, 0, 0, 0, 0, amp);
        }

        /**
          * After reading user input, the steps are:
          * (1) fork a child process
          * (2) the child process will invoke execvp()
          * (3) if command includes &, parent and child will run concurrently
          */
    }

    return 0;
}

void runProcess(char *args[], pid_t pid, char *history[], int redirect_out, int redirect_in, int out, int in, int amp)
{
    int i = 0;

    pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork Failed");
    }
    else if (pid == 0)
    {
        if (amp == 1)
        {
            setpgid(0, 0);
        }
        // printf("Executing child process\n");
        if (redirect_in == 1 && redirect_out == 1) //pipe
        {
            int fd[2];
            int pipe_index = indexOf(args, "|");
            args[indexOf(args, "|")] = '\0';
            char *args2[MAX_LINE / 2 + 1] = {'\0'};
            copyArray(args, args2, pipe_index + 1);

            if (pipe(fd) == -1)
            {
                fprintf(stderr, "Pipe Failed");
            }

            pid_t child_pid = fork();

            if (child_pid < 0)
            {
                fprintf(stderr, "Fork Failed");
            }
            else if (child_pid == 0)
            {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);

                if (execvp(args[0], args) == -1)
                {
                    perror("error executing child");
                }
            }
            else
            {
                wait(NULL);
            }

            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            if (execvp(args2[0], args2) == -1) //TODO FIX THIS
            {
                perror("error executing child");
            }
        }
        else if (redirect_out == 1)
        {
            dup2(out, STDOUT_FILENO);
        }
        else if (redirect_in == 1)
        {
            dup2(in, STDIN_FILENO);
        }
        if (execvp(args[0], args) == -1)
        {
            perror("error executing child");
        }
    }
    else
    {
        if (amp == 0)
        {
            wait(NULL);
        }
        // printf("Child process is complete\n");
    }
}

int lengthOfArgs(char *args[])
{
    int count = 0;
    int i = 0;
    while (args[i] != NULL)
    {
        count++;
        i++;
    }
    return count;
}

int indexOf(char *args[], char *delim)
{
    int i = 0;
    int index = -1;
    while (args[i] != NULL)
    {
        if (strcmp(args[i], delim) == 0)
        {
            index = i;
        }
        i++;
    }
    return index;
}

void clearArgs(char *args[], int start)
{
    while (args[start] != NULL)
    {
        args[start] = '\0';
        start++;
    }
}

void copyArray(char *args[], char *args2[], int index)
{
    int i = 0;
    while (args[index] != NULL)
    {
        args2[i] = args[index];
        i++;
        index++;
    }
}

void parseArg(char *args[], char line[])
{
    int i = 0;
    args[i] = strtok(line, " "); //grabs first "token in the strag where the delim takes place
    while (args[i] != NULL)
    {
        args[++i] = strtok(NULL, " "); //grabs the next token where there exists a delim
    }

    i = 0;
    while (args[i] != NULL)
    { //  removes new lines from strings # is there a better way to do this
        args[i++] = strtok(args[i], "\n");
    }
}
