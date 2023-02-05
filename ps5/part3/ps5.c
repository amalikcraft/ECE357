#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>

sigjmp_buf handleSIGBUS;
void sigBusHandler();
int bgrep(int argc, char *argv[], char *pattern_file, int bytes);

int main(int argc, char* argv[]){
    signal(SIGBUS, sigBusHandler);
    struct stat buf;
    int bytes = 0;
    int fd;
    char *pattern_file= (char*)malloc(BUFSIZ);
    int opt;
    while ((opt = getopt(argc, argv, "file:c:")) != -1){
        switch (opt){
        case 'p':
            if (optarg != NULL){
                fd = open(optarg, O_RDONLY);
                if (fstat(fd, &buf)){
                    fprintf(stderr, "ERROR fstat: %s\n", strerror(errno));
                }
                pattern_file = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);

                if (close(fd) < 0){
                    fprintf(stderr, "ERROR Unable to Close: %s\n", strerror(errno));
                }
            }
            else{
                pattern_file = argv[optind++];
            }
            break;

        case 'c':
            bytes = (int)strtol(optarg, NULL, 0);
            break;
            bytes = atoi(optarg);
            if (bytes == 0 && strcmp(optarg, "0")){
                fprintf(stderr, "Missing Integer after -c\n");
            }
        case '?':
            fprintf(stderr, "INVALID Option: %c\n", optopt);
            exit(-1);

        default:
            printf("INVALID SYNTAX: bgrep {OPTIONS} -file pattern_file {file1 {file2} ...}");
            exit(-1);
        }
    }
    bgrep(argc, argv, pattern_file, bytes);
}

int bgrep(int argc, char *argv[], char *pattern_file, int bytes){
    int savemask, fd, found = 0;
    struct stat buf;
    char *file;

    for (int i = optind; i < argc; i++){
        fd = open(argv[i], O_RDONLY);
        if (fd < 0){
            fprintf(stderr, "ERROR Unable to Open: %s\n", strerror(errno));
            continue;
        }
        if (fstat(fd, &buf) < 0){
            fprintf(stderr, "ERROR fstat: %s\n", strerror(errno));
            close(fd);
            continue;
        }
        file = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (file == MAP_FAILED){
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            close(fd);
            continue;
        }
        if (sigsetjmp(handleSIGBUS, savemask)){
            close(fd);
            fprintf(stderr, "ERROR: SIGBUS recieved: %s\n", strerror(errno));
            continue;
        }
        for (int j = 0; j < buf.st_size; j++){
            if (memcmp(&file[j], pattern_file, strlen(pattern_file)) == 0){
                printf("%s: %d\t", argv[i], j);
                if (bytes != 0){
                    for (int k = j - bytes; k < j + bytes + strlen(pattern_file); k++){
                        if (k < 0) k = 0;
                        printf("%c ", isprint(file[k]) ? file[k] : '?');
                    }
                    printf("\t");
                    for (int h = j - bytes; h < j + strlen(pattern_file) + bytes; h++){
                        printf("%02x ", file[h]);
                    }
                }
                printf("\n");
                found = 1;
                continue;
            }
        }
        if (!found){
            printf("Unable to find Pattern: %s .\n", argv[i]);
            found = 0;
        }
        close(fd);
        if (close(fd) < 0){
            fprintf(stderr, "ERROR Unable to Close: %s\n", strerror(errno));
        }
    }
    return found ? 0 : 1;
}

void sigBusHandler(){
fprintf(stderr, "SIGBUS received");
siglongjmp(handleSIGBUS, 1);
}
