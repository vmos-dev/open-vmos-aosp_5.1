#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/limits.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <string.h>



long long nanotime(void)
{
    struct timespec t;
    
    if(clock_gettime(CLOCK_MONOTONIC, &t)) {
        fprintf(stderr,"clock failure\n");
        exit(1);
    }

    return (((long long) t.tv_sec) * 1000000000LL) +
        ((long long) t.tv_nsec);
}

static struct timespec ts_sub(struct timespec a, struct timespec b)
{
    struct timespec r;
    r.tv_sec = a.tv_sec - b.tv_sec;
    r.tv_nsec = a.tv_nsec - b.tv_nsec;
    if(r.tv_nsec < 0) {
        r.tv_sec--;
        r.tv_nsec += 1000 * 1000 * 1000;
    }
    if(r.tv_sec < 0 && r.tv_nsec > 0) {
        r.tv_sec++;
        r.tv_nsec -= 1000 * 1000 * 1000;
    }
    return r;
}

static struct timespec ts_min(struct timespec a, struct timespec b)
{
    if(a.tv_sec < b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec))
        return a;
    else
        return b;
}

static struct timespec ts_max(struct timespec a, struct timespec b)
{
    if(a.tv_sec < b.tv_sec || (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec))
        return b;
    else
        return a;
}

int main(int argc, char **argv)
{
    long long tnow, tlast;
    struct timespec t1, dtmin, dtminp, dtmax;
    int print_interval = 50000;
    int print_countdown = 1;
    int clock_id = CLOCK_MONOTONIC;
    dtmin.tv_sec = INT_MAX;
    dtmin.tv_nsec = 0;
    dtminp.tv_sec = INT_MAX;
    dtminp.tv_nsec = 0;
    dtmax.tv_sec = 0;
    dtmax.tv_nsec = 0;
    tlast = 0;

    if(argc == 2) {
        clock_id = atoi(argv[1]);
        printf("using clock %d\n", clock_id);
    }
    clock_gettime(clock_id, &t1);
    
    for(;;) {
        struct timespec t, dt;
        clock_gettime(clock_id, &t);
        dt = ts_sub(t, t1);
        t1 = t;
        dtmin = ts_min(dtmin, dt);
        if(dt.tv_sec > 0 || dt.tv_nsec > 0)
            dtminp = ts_min(dtminp, dt);
        if(print_countdown != print_interval)
            dtmax = ts_max(dtmax, dt);
        if(--print_countdown == 0) {
            fprintf(stderr,"%09ld.%09ld, dt %ld.%09ld, min %ld.%09ld, minp %ld.%09ld, max %ld.%09ld\n",
                    t.tv_sec, t.tv_nsec, dt.tv_sec, dt.tv_nsec,
                    dtmin.tv_sec, dtmin.tv_nsec, dtminp.tv_sec, dtminp.tv_nsec,
                    dtmax.tv_sec, dtmax.tv_nsec);
            print_countdown = print_interval;
        }
    }
    for(;;) {
        tnow = nanotime();
        if(tnow < tlast) {
#if 0
            fprintf(stderr,"time went backwards: %lld -> %lld\n",
                    tlast, tnow);
            exit(1);
#endif
            fprintf(stderr,"%lld ROLLBACK\n", tnow);
        } else {
            fprintf(stderr,"%lld\n", tnow);
        }
        tlast = tnow;
    }

    return 0;
}
