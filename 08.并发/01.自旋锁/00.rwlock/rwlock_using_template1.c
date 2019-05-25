rwlock_t lock ;//定义rwlock 
rwlock_init(&lock) ;

//读时获取锁
read_lock(&lock) ;
...//临界资源
read_unlock(&lock) ;

//写时获得锁
write_lock_irqsave(&lock, flags) ;
...
write_unlock_irqrestore(&lock, flags) ;

