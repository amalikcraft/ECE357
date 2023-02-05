//Ahmad Malik
//ECE357  Fall 2022
//PS3 Problem 3

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#define BUFFER 4096


int parse(FILE *stream);
int child(char *line[], int argNum);
void myCD(int arg, char *dirPath);
void myPwd(int arg);
void myExit(int arg, int errorStat, char *exitVal);


int main(int argc, char *argv[]){
    FILE *file;
    if (argc > 2){
        fprintf(stderr,"ERROR: Invalid number of arguments\n");
    }
    else if (argc == 2){
        if ((file = fopen(argv[1],"r")) == NULL){
            fprintf(stderr, "ERROR: Unable to Open file '%s': %s.\n", argv[1], strerror(errno));
        }
        else{
            parse(file);
        }
    }
    else if (argc == 1){
        parse(stdin);
    }
    return 0;
}

int parse(FILE *stream){
    int errorStat = 0;
    char command[BUFFER];
    char *line[BUFFER];
    while (fgets(command, BUFFER, stream)){
        if (command[0] == '#' || command[0] == '\n'){
            continue;
        }
        char *token = strtok(command, " \n");  //identifying tokens
        line[0] = token;
        int i = 1;
        while (token != NULL){
            token = strtok(NULL, " \n");
            line[i] = token;
            ++i;
        }
        int argNum = i - 1;  //number of arguments per command line
        if (!strcmp(line[0], "pwd")){
            myPwd(argNum);
        }
        else if (!strcmp(line[0], "cd")){
            myCD(argNum, line[1]);
        }
        else if(!strcmp(line[0], "exit")){
            myExit(argNum, errorStat, line[1]);
        }
        else{
            pid_t pid;
            int status;
            struct rusage rusage;
            struct timeval start, end;
            //start time
            gettimeofday(&start, NULL);
            switch (pid = fork()){    //creating child process
                case -1:
                    fprintf(stderr, "ERROR: command %s: %s\n", line[0], strerror(errno));
                    exit(1);
                case 0:
                    //running child process
                    child(line, argNum);
                    break;
                default:
                    // parent process waiting
                    if (wait3(&status, 0, &rusage) == -1){
                        fprintf(stderr, "ERROR: wait failed: %s \n", strerror(errno));
                        exit(1);
                    }
                    if (status==0)
                        fprintf(stderr, "Child process: %d exited normally.\n", pid);
                    else
                    {
                        if (WIFSIGNALED(status))
                            fprintf(stderr, "Child process: %d exited with signal %d\n", pid, WTERMSIG(status));
                        else
                        {
                            errorStat = WEXITSTATUS(status);
                            fprintf(stderr, "Child process: %d exited with return value %d\n", pid, errorStat);
                        }
                    }
                    //end time
                    gettimeofday(&end, NULL);
                    fprintf(stderr, "Real: %.3fs ", (double) (end.tv_usec - start.tv_usec)/1000000);
                    fprintf(stderr, "User: %.3fs ", (double) rusage.ru_utime.tv_usec/1000000);
                    fprintf(stderr, "Sys: %.3fs \n", (double) rusage.ru_stime.tv_usec/1000000);
                    break;
            }
        }
    }
    exit(errorStat);
}
int child(char *line[], int argNum){
    int redirect = 0;
    int fd;
    char *fileName;
    for(int i = 0; i < argNum ; i++){
        if(line[i][0] == '<'){  //printing out 5 special blocks of code to check error redirection :/
            fileName = line[i] + 1;
            if ((fd = open(fileName, O_RDONLY,0666)) < 0){
                fprintf(stderr, "ERROR Unable to OPEN file '%s' : %s.\n", fileName, strerror(errno));
                exit(1);
            }
            dup2(fd,0);
            if (close (fd)!= 0) {
                fprintf(stderr, "ERROR Unable to CLOSE file '%s' : %s.\n", fileName, strerror(errno));
                exit(1);
            }
            redirect++;
        }
        else if((line[i][0]) == '>'){
            if((line[i][1]) == '>'){
                fileName = line[i] + 2;
                if(( fd = open(fileName, O_WRONLY|O_CREAT|O_APPEND,0666)) < 0){
                    fprintf(stderr, "ERROR Unable to OPEN file '%s' : %s.\n", fileName, strerror(errno));
                    exit(1);
                }
                dup2(fd,1);
                if (close (fd)!= 0){
                    fprintf(stderr, "ERROR Unable to CLOSE file '%s' : %s.\n", fileName, strerror(errno));
                    exit(1);
                }
                redirect++;
            }
            else{
                fileName = line[i] + 1;
                if ((fd=open(fileName, O_WRONLY|O_CREAT|O_TRUNC,0666)) < 0){
                    fprintf(stderr, "ERROR Unable to OPEN file '%s' : %s.\n", fileName, strerror(errno));
                    return 1;
                }
                dup2(fd,0);
                if (close (fd)!= 0){
                    fprintf(stderr, "ERROR Unable to CLOSE file '%s' : %s.\n", fileName, strerror(errno));
                    exit(1);
                }
                redirect++;
            }
        }
        else if(line[i][0] == '2' && line[i][1] == '>'){
            if((line[i][2]) == '>'){
                fileName = line[i] + 2;
                if ((fd=open(fileName, O_WRONLY|O_CREAT|O_APPEND,0666)) < 0){
                    fprintf(stderr, "ERROR Unable to OPEN file '%s' : %s.\n", fileName, strerror(errno));
                    return 1;
                }
                dup2(fd,0);
                if (close (fd)!= 0){
                        fprintf(stderr, "ERROR Unable to CLOSE file '%s' : %s.\n", fileName, strerror(errno));
                        exit(1);
                }
                redirect++;
            }
            else{
                fileName = line[i] + 3;
                if ((fd=open(fileName, O_WRONLY|O_CREAT|O_TRUNC,0666)) < 0){
                    fprintf(stderr, "ERROR Unable to OPEN file '%s' : %s.\n", fileName, strerror(errno));
                    return 1;
                }
                dup2(fd,0);
                if (close (fd)!= 0){
                        fprintf(stderr, "ERROR Unable to CLOSE file '%s' : %s.\n", fileName, strerror(errno));
                        exit(1);
                }
                redirect++;
            }
        }
    }
    //executing command
    line[argNum - redirect] = NULL;
    execvp(line[0], line);
    fprintf(stderr,"ERROR: failed to exec '%s': %s\n", line[0], strerror(errno));
    exit(127);
}

void myCD(int argNum, char *dirPath){
    if(chdir(dirPath)<0){
        fprintf(stderr, "ERROR: Unable to access directory '%s': %s\n", dirPath, strerror(errno));
    }
    if(argNum == 1){
        chdir(getenv("HOME"));
    }
    else if (argNum > 2){
        fprintf(stderr, "ERROR: Invalid number of arguments\n");
    }
}

void myPwd(int argNum){
    char dirPath[BUFFER];
    if (argNum != 1){
        fprintf(stderr, "ERROR: Invalid number of arguments\n");
    }
    else if(getcwd(dirPath, BUFFER) != NULL){
        printf("%s\n", dirPath);
    }
    else{
        fprintf(stderr, "ERROR when calling pwd: %s\n", strerror(errno));
    }
}

void myExit(int argNum, int errorStat, char *exitVal){
    if (argNum == 1){
        exit(errorStat);
    }
    else if (argNum>2){
        fprintf(stderr, "ERROR: Invalid number of arguments\n");
    }
    else{
        exit(atoi(exitVal));
    }
}
