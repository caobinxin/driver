//写执行代码模板
write_seqlock(&seqlock_a) ;
...//写操作代码块
write_sequnlock(&seqlock_a) ;

//读执行代码模板
do{
    seqnum = read_seqbegin(&seqlock_a) ;
    //读操作代码块
    ...

}while(read_seqretry(&seqlock_a, seqnum));
