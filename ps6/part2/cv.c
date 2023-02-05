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
#include "cv.h"

void handler(int signum){
    if(signum == SIGUSR1) printf("SIGUSR1 Recieved\n");
}

void cv_init(struct cv *cv){
    int * mapped_area = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0 );
    spinlock *lock = (spinlock *)(mapped_area + sizeof(spinlock));
    cv->lock = *lock;
    for (int i = 0; i < CV_MAXPROC; i++){
        cv->pids[i] = 0;
    }
    cv->count = 0;
    signal(SIGUSR1, handler);
    sigfillset(&cv->mask);
    sigdelset(&cv->mask, SIGUSR1);
}

void cv_wait(struct cv *cv, struct spinlock *mutex){
    spin_lock(&cv->lock);
    cv->pids[cv->count] = getpid();
    cv->count++;
    spin_unlock(&cv->lock);
    spin_unlock(mutex);
    sigprocmask(SIG_BLOCK, &cv->mask, NULL);
    sigsuspend(&cv->mask);
    spin_lock(&cv->lock);
    cv->count--;
    spin_unlock(&cv->lock);
    spin_lock(mutex);
}

int cv_broadcast(struct cv *cv){
    spin_lock(&cv->lock);
    int totalWake = 0;
    if (cv->count == 0) {
        spin_unlock(&cv->lock);
        return 0;
    }
    for (int i = 0; i < CV_MAXPROC; i++){
        if (cv->pids[i] > 0){
            kill(cv->pids[i], SIGUSR1);
            totalWake++;
        }
    }
    spin_unlock(&cv->lock);
    return totalWake;
}

int cv_signal(struct cv *cv){
    spin_lock(&cv->lock);
    int totalWake = 0;
    for (int i = 0; i < CV_MAXPROC; i++) {
        if (cv->pids[i] > 0) {
            kill(cv->pids[i], SIGUSR1);
            totalWake++;
            break;
        }
    }
    spin_unlock(&cv->lock);
    return totalWake;
}
