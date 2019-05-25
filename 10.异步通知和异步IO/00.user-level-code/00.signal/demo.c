#include <signal.h>
void sigterm_handler(int signo)
{
    printf("Have caught sig N.o. %d\n", signo) ;
}
int main(int argc, char const *argv[])
{
    signal(SIGINT, sigterm_handler) ;
    signal(SIGTERM, sigterm_handler) ;

    while(1) ;

    return 0;
}
