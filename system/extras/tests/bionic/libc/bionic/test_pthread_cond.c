#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


static pthread_cond_t cond1;
static pthread_cond_t cond2;
static pthread_mutex_t test_lock = PTHREAD_MUTEX_INITIALIZER;

static void *
thread1_func(void* arg)
{
    printf("Thread 1 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    printf("1 waiting for cond1\n");
    pthread_mutex_lock(&test_lock);
    pthread_cond_wait(&cond1, &test_lock );
    pthread_mutex_unlock(&test_lock);
    printf("Thread 1 done.\n");
    return 0;
}

static void *
thread2_func(void* arg)
{
    printf("Thread 2 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    printf("2 waiting for cond2\n");
    pthread_mutex_lock(&test_lock);
    pthread_cond_wait(&cond2, &test_lock );
    pthread_mutex_unlock(&test_lock);

    printf("Thread 2 done.\n");
    return 0;
}

static void *
thread3_func(void* arg)
{
    printf("Thread 3 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    printf("3 waiting for cond1\n");
    pthread_mutex_lock(&test_lock);
    pthread_cond_wait(&cond1, &test_lock );
    pthread_mutex_unlock(&test_lock);
    printf("3 Sleeping\n");
    sleep(2);
    printf("3 signal cond2\n");
    pthread_cond_signal(&cond2);

    printf("Thread 3 done.\n");
    return 0;
}

static void *
thread4_func(void* arg)
{
    printf("Thread 4 (arg=%d tid=%d) entered.\n", (unsigned)arg, gettid());
    printf("4 Sleeping\n");
    sleep(5);

    printf("4 broadcast cond1\n");
    pthread_cond_broadcast(&cond1);
    printf("Thread 4 done.\n");
    return 0;
}

int main(int argc, const char *argv[])
{
    pthread_t t[4];

    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);
    pthread_create( &t[0], NULL, thread1_func, (void *)1 );
    pthread_create( &t[1], NULL, thread2_func, (void *)2 );
    pthread_create( &t[2], NULL, thread3_func, (void *)3 );
    pthread_create( &t[3], NULL, thread4_func, (void *)4 );

    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_join(t[2], NULL);
    pthread_join(t[3], NULL);
    return 0;
}
