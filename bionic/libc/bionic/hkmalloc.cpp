
#include <sys/param.h>
#include <unistd.h>
#include <pthread.h>
#include "hkmalloc.h"
#include <__a_bionic_hook.h>
// extern "C"

extern "C" void *get_host_function(int num);

// void init_hkmalloc(KernelArgumentBlock* args)
// {
// }

void* hkcalloc(size_t n_elements, size_t elem_size)
{
    return mhkcalloc(n_elements, elem_size);
}

void hkfree(void* mem)
{
    return mhkfree(mem);
}

struct mallinfo hkmallinfo()
{
    return mhkmallinfo();
}

void* hkmalloc(size_t bytes)
{
    return mhkmalloc(bytes);
}

size_t hkmalloc_usable_size(const void* mem)
{
    return mhkmalloc_usable_size(mem);
}

void* hkmemalign(size_t alignment, size_t bytes)
{
    return mhkmemalign(alignment, bytes);
}

int hkposix_memalign(void** memptr, size_t alignment, size_t size)
{
    return mhkposix_memalign(memptr, alignment, size);
}

#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
void* hkpvalloc(size_t bytes)
{
    return mhkpvalloc(bytes);
}
#endif

void* hkrealloc(void* oldMem, size_t bytes)
{
    return mhkrealloc(oldMem, bytes);
}

#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
void* hkvalloc(size_t bytes)
{
    return mhkvalloc(bytes);
}
#endif

