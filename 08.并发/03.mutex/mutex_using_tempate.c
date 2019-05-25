struct mutex my_mutex ; //定义mutex
mutex_init(&my_mutex) ;//初始化mutex

mutex_lock(&my_mutex) ;//获取mutex

...//临界区代码

mutex_unlock(&my_mutex) ;//释放mutex


