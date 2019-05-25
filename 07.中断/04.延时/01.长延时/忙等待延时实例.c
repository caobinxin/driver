/*延时100个jiffies*/

unsigned long delay = jiffies + 100 ;
while( time_before(jiffies, delay)) ;

/*再延时2s*/
unsigned long delay = jiffies + 2*HZ ;
while(time_before(jiffies, delay)) ;
