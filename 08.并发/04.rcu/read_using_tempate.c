struct foo{
    int a ;
    int b ;
    int c ;
} ;

struct foo *gb = NULL ;

/*..写端..*/
p = kmalloc(sizeof(*p), GFP_KERNEL) ;
p->a = 1 ;
p->b = 2 ;
p->c = 3 ;
rcu_assign_pointer(gp, p) ;

/*读端访问该片区域*/
rcu_read_lock() ;
p = rcu_dereference(gp) ;
if(p != NULL)
{
    do_something_with(p->a, p->b, p->c) ;
}
rcu_read_unlock() ;

/**
 * 
 * 在上述代码中，我们可以吧写端　rcu_assign_pointer()看成发布了gp,
 * 
 * 而读端rcu_dereference看成订阅了gb,  他保证读端可以看到rcu_assign_pointer()之前所有内存被设置的情况（即　gb->a, gb->b, gp->c 等于1 2 3对于读端可见）
 *
 * 由此可见，和rcu相关的原语已经内嵌了相关的编译屏障或内存障碍 
*/