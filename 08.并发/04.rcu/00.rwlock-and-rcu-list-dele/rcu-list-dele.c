struct el{
    struct list_head lp ;
    long key ;
    spinlock_t mutex ;
    int data ;
}

DEFINE_SPINLOCK(listmutex) ;
LIST_HEAD(head) ;

int search(long key, int *result)
{
    struct el *p ;

    rcu_read_lock() ;
    list_for_each_entry_rcu(p, &head, lp){
        if(p->key == key){
            *result = p->data ;
            rcu_read_unlock() ;
            return 1 ;
        }
    }

    rcu_read_unlock() ;
    return 0 ;
}

int delete(long key)
{
    struct el *p ;

    spin_lock(&listmutex) ;
    list_for_each_entry(p, &head, lp){

        if(p-key == key){
            list_del_rcu(&p->lp) ;
            spin_unlock(&listmutex) ;
            synchronize_rcu() ;//该函数由rcu写执行单元调用，它将阻塞写执行单元，直到当前cpu上所有的已经存在的读执行单元完成读临界区，写执行单元才能继续下一步的操作
            kfree(p);
            return 1 ;
        }
    }
    spin_unlock(&listmutex) ;
    return 0 ;
}