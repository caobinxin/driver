//定义semaphore
DECLARE_MUTEX(mount_sem) ;

down(&mount_sem) ;//获取semaphore
...
up(&mount_sem) ;//释放semaphore
