//定义一个自旋锁
spinlock_t lock ;
spin_lock_init(&lock) ;
spin_lock(&lock) ;//获得自旋锁，保护临界区
...//临界区
spin_unlock(&lock) ;//解锁
