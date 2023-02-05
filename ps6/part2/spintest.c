#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdatomic.h>

#include "spinlock.h"

int main(int argc, char *argv[]){
    if (argc != 3){
      fprintf(stderr, "ERROR invalid number of arguments\n");
      exit(0);
    }
    int nchild = strtoull(argv[1], NULL, 10);
    int niter = strtoull(argv[2], NULL, 10);
    printf("nchild = %d\n", nchild);
    printf("niter = %d\n", niter);

    int *region = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    assert(region != MAP_FAILED);

    region[0] = 0;
    spinlock *lock = (spinlock *)(region + sizeof(spinlock));
    lock->slock = region[1];

    pid_t pid;
    for (int i = 0; i < nchild; i++){
        if ((pid = fork()) < 0) {
            fprintf(stderr, "ERROR failed to fork: %s\n", i, strerror(errno));
            return EXIT_FAILURE;
        }
        if (pid == 0) {
            spin_lock(lock);
            for (int j = 0; j < niter; j++){
                atomic_fetch_add(&region[0], 1);
            }
            spin_unlock(lock);
            exit(0);
        }
    }
    while(wait(NULL)>0);
    printf("%d\n", region[0]);
}
