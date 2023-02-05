typedef struct spinlock{
	volatile char slock;
}spinlock;

void spin_lock(struct spinlock *l);
void spin_unlock(struct spinlock *l);
int tas(volatile char *lock);
