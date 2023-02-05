//Ahmad Malik
//ECE357 Fall 2022
//Problem Set 4
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#define BUFFSIZE 4096
jmp_buf jumpBuf;
int totalBytes = 0;
int totalFiles = 0;

void handler_Sig1(int signum){
    fprintf(stderr, "Interrupt Caught: %d\n", signum);
    fprintf(stderr,"Total files processed: %d,\nTotal bytes processed: %d\n",  totalFiles, totalBytes);
    longjmp(jumpBuf, 1);
}
void handler_Sig2(int signum){
    fprintf(stderr, "Interrupt Caught: %d\n", signum);
    fprintf(stderr,"*** SIGUSR2 received, moving on to file #%d\n", totalFiles + 1);
    longjmp(jumpBuf, 1);
}

int main(int argc, char * argv[]){
    int grep_pipe[2], more_pipe[2];
    if (argc<3){
        fprintf(stderr, "Incorrect Formatting: ./catgrepmore pattern infile1 [...infile2...]\n");
        exit(EXIT_FAILURE);
    }
    struct sigaction sigact1;
    sigact1.sa_handler = handler_Sig1;
    sigact1.sa_mask = 0;
    sigact1.sa_flags = 0;
    if (sigaction(SIGINT, &sigact1, 0) < 0){
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
    }
    struct sigaction sigact2;
    sigact2.sa_handler = handler_Sig2;
    sigact2.sa_mask = 0;
    sigact2.sa_flags = 0;
    if (sigaction(SIGPIPE, &sigact2, 0) < 0){
        fprintf(stderr, "ERROR: %s\n", strerror(errno));
    }
    for (int i = 2; i < argc; i++){
        if(setjmp(jumpBuf) != 0){
            continue;
        }
        int grep_pid = fork();
        if(grep_pid < 0){
            fprintf(stderr, "ERROR: unable to fork 'grep': %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if(grep_pid == 0){
            if (dup2(grep_pipe[0], 0) < 0){
                fprintf(stderr, "ERROR: Unable to duplicate grep-pipe 'WRITE': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (dup2(more_pipe[1], 0) < 0){
                fprintf(stderr, "ERROR: Unable to duplicate more-pipe 'READ': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (execlp("grep", "grep", argv[1],NULL) < 0){
                fprintf(stderr, "ERROR: Unable to execute grep: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (close(grep_pipe[1]) < 0){
                fprintf(stderr, "ERROR: Unable to close grep-pipe 'READ': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (close(more_pipe[0]) < 0){
                fprintf(stderr, "ERROR: Unable to close more-pipe 'WRITE': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        else{
            fprintf(stderr, "ERROR: Unable to close pipes: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        int more_pid = fork();
        if (more_pid < 0){
            fprintf(stderr, "ERROR: Unable to fork 'more': %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if(more_pid == 0){
            if(dup2(more_pipe[0], 0) < 0){
                fprintf(stderr, "ERROR: Unable to duplicate stdin to more-pipe: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if(execlp("more", "more", NULL) < 0){
                fprintf(stderr, "ERROR: Unable to execute 'more': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if(close(grep_pipe[1]) < 0){
                fprintf(stderr, "ERROR: Unable to close grep-pipe 'WRITE': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        else{
            int fd;
            char *buf = malloc(BUFFSIZE);
            while((read(fd, buf, BUFFSIZE))){
                int bytesRead = read(fd, buf, BUFFSIZE);
                if (bytesRead < 0){
    				if (errno == EINTR){
                        fprintf(stderr, "ERROR: Unable to read file: %s\n", strerror(errno));
    					continue;
    				}
                }
                totalBytes = totalBytes + bytesRead;

                int bytesWritten = 0;
                while (bytesWritten < bytesRead){
                    int temp = write(grep_pipe[1], buf, bytesRead);
                    if (temp <= 0){
                        fprintf(stderr, "ERROR: file only partially written to: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    bytesWritten = bytesWritten + temp;
                }
            }

            totalFiles++;
            if(close(grep_pipe[1]) < 0){
                fprintf(stderr, "ERROR: ERROR: Unable to close grep-pipe 'WRITE': %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            if(close(fd) < 0){
                fprintf(stderr, "ERROR: Unable to close file %s : %s\n", argv[i], strerror(errno));
                exit(EXIT_FAILURE);
            }

            int grep_status, more_status;
            waitpid(grep_pid, &grep_status, 0);
            waitpid(more_pid, &more_status, 0);
        }
    }
    return 0;
}
