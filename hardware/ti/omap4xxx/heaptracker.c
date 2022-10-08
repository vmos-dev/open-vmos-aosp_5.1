/*
**
** Copyright (C) 2008-2011, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <android/log.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>

#include "mapinfo.h"

extern int heaptracker_stacktrace(intptr_t*, size_t);
extern void *__real_malloc(size_t size);
extern void *__real_realloc(void *ptr, size_t size);
extern void *__real_calloc(int nmemb, int size);
extern void __real_free(void *ptr);

static mapinfo *milist;

#define MAX_BACKTRACE_DEPTH 15
#define ALLOCATION_TAG      0x1ee7d00d
#define BACKLOG_TAG         0xbabecafe
#define FREE_POISON         0xa5
#define BACKLOG_MAX         50
#define FRONT_GUARD         0xaa
#define FRONT_GUARD_LEN     (1<<4)
#define REAR_GUARD          0xbb
#define REAR_GUARD_LEN      (1<<4)
#define SCANNER_SLEEP_S     3

struct hdr {
    uint32_t tag;
    struct hdr *prev;
    struct hdr *next;
    intptr_t bt[MAX_BACKTRACE_DEPTH];
    int bt_depth;
    intptr_t freed_bt[MAX_BACKTRACE_DEPTH];
    int freed_bt_depth;
    size_t size;
    char front_guard[FRONT_GUARD_LEN];
} __attribute__((packed));

struct ftr {
    char rear_guard[REAR_GUARD_LEN];
} __attribute__((packed));

static inline struct ftr * to_ftr(struct hdr *hdr)
{
    return (struct ftr *)(((char *)(hdr + 1)) + hdr->size);
}

static inline void *user(struct hdr *hdr)
{
    return hdr + 1;
}

static inline struct hdr *meta(void *user)
{
    return ((struct hdr *)user) - 1;
}

extern int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);
static void default_log(const char *fmt, ...)
{
    va_list lst;
    va_start(lst, fmt);
    __android_log_vprint(ANDROID_LOG_ERROR, "DEBUG", fmt, lst);
    va_end(lst);
}

/* Override this for non-printf reporting */
void (*malloc_log)(const char *fmt, ...) = default_log;
/* Call this ad dlclose() to get leaked memory */
void free_leaked_memory(void);

static unsigned num;
static struct hdr *first;
static struct hdr *last;
static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

static unsigned backlog_num;
static struct hdr *backlog_first;
static struct hdr *backlog_last;
static pthread_rwlock_t backlog_lock = PTHREAD_RWLOCK_INITIALIZER;

void print_backtrace(const intptr_t *bt, int depth)
{
    mapinfo *mi;
    int cnt, rel_pc;
    intptr_t self_bt[MAX_BACKTRACE_DEPTH];

    if (!bt) {
        depth = heaptracker_stacktrace(self_bt, MAX_BACKTRACE_DEPTH);
        bt = self_bt;
    }

    malloc_log("*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n");
    for (cnt = 0; cnt < depth && cnt < MAX_BACKTRACE_DEPTH; cnt++) {
        mi = pc_to_mapinfo(milist, bt[cnt], &rel_pc);
        malloc_log("\t#%02d  pc %08x  %s\n", cnt,
                   mi ? rel_pc : bt[cnt],
                   mi ? mi->name : "(unknown)");
    }
}

static inline void init_front_guard(struct hdr *hdr)
{
    memset(hdr->front_guard, FRONT_GUARD, FRONT_GUARD_LEN);
}

static inline int is_front_guard_valid(struct hdr *hdr)
{
    unsigned i;
    for (i = 0; i < FRONT_GUARD_LEN; i++)
        if (hdr->front_guard[i] != FRONT_GUARD)
            return 0;
    return 1;
}

static inline void init_rear_guard(struct hdr *hdr)
{
    struct ftr *ftr = to_ftr(hdr);
    memset(ftr->rear_guard, REAR_GUARD, REAR_GUARD_LEN);
}

static inline int is_rear_guard_valid(struct hdr *hdr)
{
    unsigned i;
    int valid = 1;
    int first_mismatch = -1;
    struct ftr *ftr = to_ftr(hdr);
    for (i = 0; i < REAR_GUARD_LEN; i++) {
        if (ftr->rear_guard[i] != REAR_GUARD) {
            if (first_mismatch < 0)
                first_mismatch = i;
            valid = 0;
        }
        else if (first_mismatch >= 0) {
            malloc_log("+++ REAR GUARD MISMATCH [%d, %d)\n", first_mismatch, i);
            first_mismatch = -1;
        }
    }

    if (first_mismatch >= 0)
        malloc_log("+++ REAR GUARD MISMATCH [%d, %d)\n", first_mismatch, i);
    return valid;
}

static inline void __add(struct hdr *hdr, struct hdr **first, struct hdr **last)
{
    hdr->prev = 0;
    hdr->next = *last;
    if (*last)
        (*last)->prev = hdr;
    else
        *first = hdr;
    *last = hdr;
}

static inline int __del(struct hdr *hdr, struct hdr **first, struct hdr **last)
{
    if (hdr->prev)
        hdr->prev->next = hdr->next;
    else
        *last = hdr->next;
    if (hdr->next)
        hdr->next->prev = hdr->prev;
    else
        *first = hdr->prev;
    return 0;
}

static inline void add(struct hdr *hdr, size_t size)
{
    pthread_rwlock_wrlock(&lock);
    hdr->tag = ALLOCATION_TAG;
    hdr->size = size;
    init_front_guard(hdr);
    init_rear_guard(hdr);
    num++;
    __add(hdr, &first, &last);
    pthread_rwlock_unlock(&lock);
}

static inline int del(struct hdr *hdr)
{
    if (hdr->tag != ALLOCATION_TAG)
        return -1;

    pthread_rwlock_wrlock(&lock);
    __del(hdr, &first, &last);
    num--;
    pthread_rwlock_unlock(&lock);
    return 0;
}

static inline void poison(struct hdr *hdr)
{
    memset(user(hdr), FREE_POISON, hdr->size);
}

static int was_used_after_free(struct hdr *hdr)
{
    unsigned i;
    const char *data = (const char *)user(hdr);
    for (i = 0; i < hdr->size; i++)
        if (data[i] != FREE_POISON)
            return 1;
    return 0;
}

/* returns 1 if valid, *safe == 1 if safe to dump stack */
static inline int check_guards(struct hdr *hdr, int *safe)
{
    *safe = 1;
    if (!is_front_guard_valid(hdr)) {
        if (hdr->front_guard[0] == FRONT_GUARD) {
            malloc_log("+++ ALLOCATION %p SIZE %d HAS A CORRUPTED FRONT GUARD\n",
                       user(hdr), hdr->size);
        } else {
            malloc_log("+++ ALLOCATION %p HAS A CORRUPTED FRONT GUARD "\
                      "(NOT DUMPING STACKTRACE)\n", user(hdr));
            /* Allocation header is probably corrupt, do not print stack trace */
            *safe = 0;
        }
        return 0;
    }

    if (!is_rear_guard_valid(hdr)) {
        malloc_log("+++ ALLOCATION %p SIZE %d HAS A CORRUPTED REAR GUARD\n",
                   user(hdr), hdr->size);
        return 0;
    }

    return 1;
}

/* returns 1 if valid, *safe == 1 if safe to dump stack */
static inline int __check_allocation(struct hdr *hdr, int *safe)
{
    int valid = 1;
    *safe = 1;

    if (hdr->tag != ALLOCATION_TAG && hdr->tag != BACKLOG_TAG) {
        malloc_log("+++ ALLOCATION %p HAS INVALID TAG %08x (NOT DUMPING STACKTRACE)\n",
                   user(hdr), hdr->tag);
	/* Allocation header is probably corrupt, do not dequeue or dump stack
         * trace.
         */
        *safe = 0;
        return 0;
    }

    if (hdr->tag == BACKLOG_TAG && was_used_after_free(hdr)) {
        malloc_log("+++ ALLOCATION %p SIZE %d WAS USED AFTER BEING FREED\n",
                   user(hdr), hdr->size);
        valid = 0;
	/* check the guards to see if it's safe to dump a stack trace */
        (void)check_guards(hdr, safe);
    }
    else
        valid = check_guards(hdr, safe);

    if (!valid && *safe) {
        malloc_log("+++ ALLOCATION %p SIZE %d ALLOCATED HERE:\n",
                        user(hdr), hdr->size);
        print_backtrace(hdr->bt, hdr->bt_depth);
        if (hdr->tag == BACKLOG_TAG) {
            malloc_log("+++ ALLOCATION %p SIZE %d FREED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(hdr->freed_bt, hdr->freed_bt_depth);
        }
    }

    return valid;
}

static inline int __del_and_check(struct hdr *hdr,
                                   struct hdr **first, struct hdr **last, unsigned *cnt,
                                   int *safe)
{
    int valid;
    valid = __check_allocation(hdr, safe);
    if (safe) {
        (*cnt)--;
        __del(hdr, first, last);
    }
    return valid;
}

static inline void __del_from_backlog(struct hdr *hdr)
{
        int safe;
        (void)__del_and_check(hdr,
                              &backlog_first, &backlog_last, &backlog_num,
                              &safe);
        hdr->tag = 0; /* clear the tag */
}

static inline void del_from_backlog(struct hdr *hdr)
{
    pthread_rwlock_wrlock(&backlog_lock);
    __del_from_backlog(hdr);
    pthread_rwlock_unlock(&backlog_lock);
}

static inline int del_leak(struct hdr *hdr, int *safe)
{
    int valid;
    pthread_rwlock_wrlock(&lock);
    valid = __del_and_check(hdr,
                            &first, &last, &num,
                            safe);
    pthread_rwlock_unlock(&lock);
    return valid;
}

static inline void add_to_backlog(struct hdr *hdr)
{
    pthread_rwlock_wrlock(&backlog_lock);
    hdr->tag = BACKLOG_TAG;
    backlog_num++;
    __add(hdr, &backlog_first, &backlog_last);
    poison(hdr);
    /* If we've exceeded the maximum backlog, clear it up */
    while (backlog_num > BACKLOG_MAX) {
        struct hdr *gone = backlog_first;
        __del_from_backlog(gone);
        __real_free(gone);
    }
    pthread_rwlock_unlock(&backlog_lock);
}

void* __wrap_malloc(size_t size)
{
//  malloc_tracker_log("%s: %s\n", __FILE__, __FUNCTION__);
    struct hdr *hdr = __real_malloc(sizeof(struct hdr) + size +
                                    sizeof(struct ftr));
    if (hdr) {
        hdr->bt_depth = heaptracker_stacktrace(
                            hdr->bt, MAX_BACKTRACE_DEPTH);
        add(hdr, size);
        return user(hdr);
    }
    return NULL;
}

void __wrap_free(void *ptr)
{
    struct hdr *hdr;
    if (!ptr) /* ignore free(NULL) */
        return;

    hdr = meta(ptr);

    if (del(hdr) < 0) {
        intptr_t bt[MAX_BACKTRACE_DEPTH];
        int depth;
        depth = heaptracker_stacktrace(bt, MAX_BACKTRACE_DEPTH);
        if (hdr->tag == BACKLOG_TAG) {
            malloc_log("+++ ALLOCATION %p SIZE %d BYTES MULTIPLY FREED!\n",
                       user(hdr), hdr->size);
            malloc_log("+++ ALLOCATION %p SIZE %d ALLOCATED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(hdr->bt, hdr->bt_depth);
            /* hdr->freed_bt_depth should be nonzero here */
            malloc_log("+++ ALLOCATION %p SIZE %d FIRST FREED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(hdr->freed_bt, hdr->freed_bt_depth);
            malloc_log("+++ ALLOCATION %p SIZE %d NOW BEING FREED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(bt, depth);
        }
        else {
            malloc_log("+++ ALLOCATION %p IS CORRUPTED OR NOT ALLOCATED VIA TRACKER!\n",
                       user(hdr));
            print_backtrace(bt, depth);
            /* Leak here so that we do not crash */
            //__real_free(user(hdr));
        }
    }
    else {
        hdr->freed_bt_depth = heaptracker_stacktrace(hdr->freed_bt,
                                      MAX_BACKTRACE_DEPTH);
        add_to_backlog(hdr);
    }
}

void *__wrap_realloc(void *ptr, size_t size)
{
    struct hdr *hdr;

    if (!size) {
        __wrap_free(ptr);
        return NULL;
    }

    if (!ptr)
        return __wrap_malloc(size);

    hdr = meta(ptr);

//  malloc_log("%s: %s\n", __FILE__, __FUNCTION__);
    if (del(hdr) < 0) {
        intptr_t bt[MAX_BACKTRACE_DEPTH];
        int depth;
        depth = heaptracker_stacktrace(bt, MAX_BACKTRACE_DEPTH);
        if (hdr->tag == BACKLOG_TAG) {
            malloc_log("+++ REALLOCATION %p SIZE %d OF FREED MEMORY!\n",
                       user(hdr), size, hdr->size);
            malloc_log("+++ ALLOCATION %p SIZE %d ALLOCATED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(hdr->bt, hdr->bt_depth);
            /* hdr->freed_bt_depth should be nonzero here */
            malloc_log("+++ ALLOCATION %p SIZE %d FIRST FREED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(hdr->freed_bt, hdr->freed_bt_depth);
            malloc_log("+++ ALLOCATION %p SIZE %d NOW BEING REALLOCATED HERE:\n",
                       user(hdr), hdr->size);
            print_backtrace(bt, depth);

	    /* We take the memory out of the backlog and fall through so the
	     * reallocation below succeeds.  Since we didn't really free it, we
	     * can default to this behavior.
             */
            del_from_backlog(hdr);
        }
        else {
            malloc_log("+++ REALLOCATION %p SIZE %d IS CORRUPTED OR NOT ALLOCATED VIA TRACKER!\n",
                       user(hdr), size);
            print_backtrace(bt, depth);
            // just get a whole new allocation and leak the old one
            return __real_realloc(0, size);
            // return __real_realloc(user(hdr), size); // assuming it was allocated externally
        }
    }
 
    hdr = __real_realloc(hdr, sizeof(struct hdr) + size + sizeof(struct ftr));
    if (hdr) {
        hdr->bt_depth = heaptracker_stacktrace(hdr->bt, MAX_BACKTRACE_DEPTH);
        add(hdr, size);
        return user(hdr);
    }

    return NULL;
}

void *__wrap_calloc(int nmemb, size_t size)
{
//  malloc_tracker_log("%s: %s\n", __FILE__, __FUNCTION__);
    struct hdr *hdr;
    size_t __size = nmemb * size;
    hdr = __real_calloc(1, sizeof(struct hdr) + __size + sizeof(struct ftr));
    if (hdr) {
        hdr->bt_depth = heaptracker_stacktrace(
                            hdr->bt, MAX_BACKTRACE_DEPTH);
        add(hdr, __size);
        return user(hdr);
    }
    return NULL;
}

void heaptracker_free_leaked_memory(void)
{
    struct hdr *del; int cnt;

    if (num)
        malloc_log("+++ THERE ARE %d LEAKED ALLOCATIONS\n", num);

    while (last) {
        int safe;
        del = last;
        malloc_log("+++ DELETING %d BYTES OF LEAKED MEMORY AT %p (%d REMAINING)\n",
                del->size, user(del), num);
        if (del_leak(del, &safe)) {
            /* safe == 1, because the allocation is valid */
            malloc_log("+++ ALLOCATION %p SIZE %d ALLOCATED HERE:\n",
                        user(del), del->size);
            print_backtrace(del->bt, del->bt_depth);
        }
        __real_free(del);
    }

//  malloc_log("+++ DELETING %d BACKLOGGED ALLOCATIONS\n", backlog_num);
    while (backlog_last) {
	del = backlog_first;
        del_from_backlog(del);
        __real_free(del);
    }
}

static int check_list(struct hdr *list, pthread_rwlock_t *rwlock)
{
    struct hdr *hdr;
    int safe, num_checked;

    pthread_rwlock_rdlock(rwlock);
    num_checked = 0;
    hdr = list;
    while (hdr) {
        (void)__check_allocation(hdr, &safe);
        hdr = hdr->next;
        num_checked++;
    }
    pthread_rwlock_unlock(rwlock);

    return num_checked;
}

static pthread_t scanner_thread;
static pthread_cond_t scanner_cond = PTHREAD_COND_INITIALIZER;
static int scanner_stop;
static pthread_mutex_t scanner_lock = PTHREAD_MUTEX_INITIALIZER;

static void* scanner(void *data __attribute__((unused)))
{
    struct timespec ts;
    int num_checked, num_checked_backlog;

    while (1) {
        num_checked = check_list(last, &lock);
        num_checked_backlog = check_list(backlog_last, &backlog_lock);

//      malloc_log("@@@ scanned %d/%d allocs and %d/%d freed\n",
//                 num_checked, num,
//                 num_checked_backlog, backlog_num);

        pthread_mutex_lock(&scanner_lock);
        if (!scanner_stop) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += SCANNER_SLEEP_S;
            pthread_cond_timedwait(&scanner_cond, &scanner_lock, &ts);
        }
        if (scanner_stop) {
            pthread_mutex_unlock(&scanner_lock);
            break;
        }
        pthread_mutex_unlock(&scanner_lock);
    }

//  malloc_log("@@@ scanner thread exiting");
    return NULL;
}

static void init(void) __attribute__((constructor));
static void init(void)
{
//  malloc_log("@@@ start scanner thread");
    milist = init_mapinfo(getpid());
    pthread_create(&scanner_thread,
                   NULL,
                   scanner,
                   NULL);
}

static void deinit(void) __attribute__((destructor));
static void deinit(void)
{
//  malloc_log("@@@ signal stop to scanner thread");
    pthread_mutex_lock(&scanner_lock);
    scanner_stop = 1;
    pthread_cond_signal(&scanner_cond);
    pthread_mutex_unlock(&scanner_lock);
//  malloc_log("@@@ wait for scanner thread to exit");
    pthread_join(scanner_thread, NULL);
//  malloc_log("@@@ scanner thread stopped");

    heaptracker_free_leaked_memory();
    deinit_mapinfo(milist);
}
