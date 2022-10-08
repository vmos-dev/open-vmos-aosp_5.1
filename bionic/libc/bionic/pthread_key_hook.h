
#ifndef LIBC_BIONIC_PTHREAD_KEY_HOOK_H_
#define LIBC_BIONIC_PTHREAD_KEY_HOOK_H_
#include <private/bionic_config.h>

#include "private/KernelArgumentBlock.h"
#include <pthread.h>

#include "private/bionic_tls.h"
#include "pthread_internal.h"

__BEGIN_DECLS

void init_pthread_key(KernelArgumentBlock* args);

typedef int (*pfnpthread_key_create)(pthread_key_t* key, void (*key_destructor)(void*));
typedef int (*pfnpthread_key_delete)(pthread_key_t key);
typedef void* (*pfnpthread_getspecific)(pthread_key_t key);
typedef int (*pfnpthread_setspecific)(pthread_key_t key, const void* ptr);


__END_DECLS

#endif // LIBC_BIONIC_PTHREAD_KEY_HOOK_H_
