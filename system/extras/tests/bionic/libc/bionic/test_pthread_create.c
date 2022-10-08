#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static void *
thread1_func(void* arg)
{
    printf("Thread 1 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    return 0;
}

static void *
thread2_func(void* arg)
{
    printf("thread 2 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    return 1;
}


int main( void )
{
    pthread_t t1, t2;

    pthread_create( &t1, NULL, thread1_func, (void *)1 );

    pthread_join(t1, NULL);

    printf("OK\n");
    return 0;
}
