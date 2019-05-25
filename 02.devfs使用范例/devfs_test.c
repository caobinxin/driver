static devfs_handle_t devfs_handle ;

static int __init xxx_init(void)
{
    int ret ;
    int i ;

    ret = register_chrdev(xxx_MAJOR, DEVICE_NAME, &xxx_fops) ;
    if( ret < 0){
        printk("can't register major number\n") ;
    }

    devfs_handle = devfs_register(NULL, DEVICE_NAME, DEVFS_FL_DEFAULT, xxx_MAJOR, 0, S_IFCHR | S_IRUSER | S_IWUSR, &xxx_fops, NULL) ;

    printk("initialized\n") ;
    return 0;
}

static void __exit xxx_exit(void)
{
    devfs_unregister(devfs_handle) ;
    unregister_chrdev(xxx_MAJOR, DEVICE_NAME);
}

module_init(xxx_init) ;
module_exit(xxx_exit) ;

