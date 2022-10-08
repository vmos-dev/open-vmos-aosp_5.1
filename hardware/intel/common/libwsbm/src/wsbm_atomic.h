/**
 * This file is under no copyright claims due to its
 * simplicity.
 */

#ifndef _WSBM_ATOMIC_H_
#define _WSBM_ATOMIC_H_

#include <stdint.h>

struct _WsbmAtomic
{
    int32_t count;
};

#define wsbmAtomicInit(_i) {(i)}
#define wsbmAtomicSet(_v, _i) (((_v)->count) = (_i))
#define wsbmAtomicRead(_v) ((_v)->count)

static inline int
wsbmAtomicIncZero(struct _WsbmAtomic *v)
{
    unsigned char c;
    __asm__ __volatile__("lock; incl %0; sete %1":"+m"(v->count), "=qm"(c)
			 ::"memory");

    return c != 0;
}

static inline int
wsbmAtomicDecNegative(struct _WsbmAtomic *v)
{
    unsigned char c;
    int i = -1;
    __asm__ __volatile__("lock; addl %2,%0; sets %1":"+m"(v->count), "=qm"(c)
			 :"ir"(i):"memory");

    return c;
}

static inline int
wsbmAtomicDecZero(struct _WsbmAtomic *v)
{
    unsigned char c;

    __asm__ __volatile__("lock; decl %0; sete %1":"+m"(v->count), "=qm"(c)
			 ::"memory");

    return c != 0;
}

static inline void
wsbmAtomicInc(struct _WsbmAtomic *v)
{
    __asm__ __volatile__("lock; incl %0":"+m"(v->count));
}

static inline void
wsbmAtomicDec(struct _WsbmAtomic *v)
{
    __asm__ __volatile__("lock; decl %0":"+m"(v->count));
}

static inline int32_t
wsbmAtomicCmpXchg(volatile struct _WsbmAtomic *v, int32_t old, int32_t new)
{
    int32_t previous;

    __asm__ __volatile__("lock; cmpxchgl %k1,%2":"=a"(previous)
			 :"r"(new), "m"(v->count), "0"(old)
			 :"memory");

    return previous;
}

#endif
