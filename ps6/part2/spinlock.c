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

void spin_lock(struct spinlock *l){
	while(tas(&(l->slock))!=0);
}

void spin_unlock(struct spinlock *l){
	l->slock=0;
}
