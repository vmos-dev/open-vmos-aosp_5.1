#include <stdio.h>
#include <sys/time.h>

extern void icache_test(long count, long step);
extern void icache_test2(long count);

int main() 
{
    printf("[bytes]\t[us]\n");

    struct timeval now, tm;
    long long t;
    long MBs;
    long i;
    long step = 32;
    for (i=0 ; step<=2048 ; i++, step+=32) 
    {
        long value;
        gettimeofday(&now, 0);
        icache_test(0x800000L, step);
        gettimeofday(&tm, 0);
        t = (tm.tv_sec*1000000LL+tm.tv_usec) - (now.tv_sec*1000000LL+now.tv_usec);
        printf("%6ld\t%lld\n", step*32, t);
    }

    gettimeofday(&now, 0);
    icache_test2(0x800000L / 2048);
    gettimeofday(&tm, 0);
    t = (tm.tv_sec*1000000LL+tm.tv_usec) - (now.tv_sec*1000000LL+now.tv_usec);
    MBs = (8388608LL*32*1000000) / (t * (1024*1024));
    printf("\n%6lld us\t%ld MB/s\n", t, MBs);
    
    return 0;
}
