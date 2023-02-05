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
#include "fifo.h"

void fifo_init(struct fifo *f){
    cv* read = (cv*)mmap(NULL, sizeof(cv), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    cv* write = (cv*)mmap(NULL, sizeof(cv), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    f->fifo_rd = *read;
    f->fifo_wr = *write;
    cv_init(&f->fifo_rd);
    cv_init(&f->fifo_wr);
	f->fifo_lock.primitive_lock = 0;
    f->next_write = 0;
    f->next_read = 0;
    f->item_count = 0;

}

void fifo_wr(struct fifo *f, unsigned long d) {
    spin_lock(&f->fifo_lock);
    while (f->item_count >= MYFIFO_BUFSIZ) {
        cv_wait(&f->fifo_wr, &f->fifo_lock);
    }
    f->buf[f->next_write++] = d;
    f->next_write %= MYFIFO_BUFSIZ;
    f->item_count++;
    cv_signal(&f->fifo_rd);
    spin_unlock(&f->fifo_lock);
}

unsigned long fifo_rd(struct fifo *f) {
    unsigned long item;
    spin_lock(&f->fifo_lock);
    while (f->item_count <= 0) {
        cv_wait(&f->fifo_rd, &f->fifo_lock);
    }
    item = f->buf[f->next_read++];
    f->next_read %= MYFIFO_BUFSIZ;
    f->item_count--;
    cv_signal(&f->fifo_wr);
    spin_unlock(&f->fifo_lock);
    return item;
}
