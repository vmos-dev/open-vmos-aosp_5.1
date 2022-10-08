
#ifndef LIBC_BIONIC_HKMALLOC_H_
#define LIBC_BIONIC_HKMALLOC_H_
#include <private/bionic_config.h>

// #include "private/KernelArgumentBlock.h"

// extern "C"
__BEGIN_DECLS

// void init_hkmalloc(KernelArgumentBlock* args);

void* hkcalloc(size_t n_elements, size_t elem_size);

void hkfree(void* mem);

struct mallinfo hkmallinfo();

void* hkmalloc(size_t bytes);

size_t hkmalloc_usable_size(const void* mem);

void* hkmemalign(size_t alignment, size_t bytes);

int hkposix_memalign(void** memptr, size_t alignment, size_t size);

#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
void* hkpvalloc(size_t bytes);
#endif

void* hkrealloc(void* oldMem, size_t bytes);

#if defined(HAVE_DEPRECATED_MALLOC_FUNCS)
void* hkvalloc(size_t bytes);
#endif

__END_DECLS




#endif  // LIBC_BIONIC_HKMALLOC_H_
