### 说明
由于spinlock是一种忙等锁，所以在保护的临界区中不太适合调用　引起休眠的函数　copy_from_user()、copy_to_user()、kmalloc()等函数

实现场景：
    读写互斥，当判断
