#ifndef KERNEL_ARGUMENT_BLOCK_H
#define KERNEL_ARGUMENT_BLOCK_H

#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <sys/auxv.h>

#include "private/bionic_macros.h"


struct Linkerdlopen
{
	unsigned long version;
	unsigned long at_base;
	unsigned long libdl_symtab;
	unsigned long hostmain;
};

#define getLinkerdlopen(val) ((struct Linkerdlopen *)(val & ~((unsigned long )0xfff)))

struct abort_msg_t;

// When the kernel starts the dynamic linker, it passes a pointer to a block
// of memory containing argc, the argv array, the environment variable array,
// and the array of ELF aux vectors. This class breaks that block up into its
// constituents for easy access.
class KernelArgumentBlock {
 public:
  KernelArgumentBlock(void* raw_args) {
    uintptr_t* args = reinterpret_cast<uintptr_t*>(raw_args);
    argc = static_cast<int>(*args);
    argv = reinterpret_cast<char**>(args + 1);
    envp = argv + argc + 1;

    // Skip over all environment variable definitions to find the aux vector.
    // The end of the environment block is marked by a NULL pointer.
    char** p = envp;
    while (*p != NULL) {
      ++p;
    }
    ++p; // Skip the NULL itself.

    auxv = reinterpret_cast<ElfW(auxv_t)*>(p);

  }

  // Similar to ::getauxval but doesn't require the libc global variables to be set up,
  // so it's safe to call this really early on.
  unsigned long getauxval(unsigned long type,bool* found_match = NULL) {
    for (ElfW(auxv_t)* v = auxv; v->a_type != AT_NULL; ++v) {
      if (v->a_type == type) {
        if (found_match != NULL) {
            *found_match = true;
        }
		if (v->a_type==AT_BASE && (v->a_un.a_val&0x100) )
		{
			return (unsigned long)getLinkerdlopen(v->a_un.a_val)->at_base;
		}
        return v->a_un.a_val;
      }
    }
    if (found_match != NULL) {
      *found_match = false;
    }
    return 0;
  }

  unsigned long getLinkerdlopenAddr() {
    for (ElfW(auxv_t)* v = auxv; v->a_type != AT_NULL; ++v) {
      if (v->a_type == AT_BASE) {
        if (v->a_type==AT_BASE && (v->a_un.a_val&0x100) )
        {
          return (unsigned long)getLinkerdlopen(v->a_un.a_val);
        }
        return 0;
      }
    }
    return 0;
  }

  unsigned long getlibdl_version() {
    for (ElfW(auxv_t)* v = auxv; v->a_type != AT_NULL; ++v) {
      if (v->a_type == AT_BASE) {
        if (v->a_type == AT_BASE && (v->a_un.a_val & 0x100)) {
          return (unsigned long)getLinkerdlopen(v->a_un.a_val)->version;
        }
        return 0;
      }
    }
    return 0;
  }


  unsigned long getlibdl_symtab() {
    for (ElfW(auxv_t)* v = auxv; v->a_type != AT_NULL; ++v) {
      if (v->a_type == AT_BASE) {
        if (v->a_type == AT_BASE && (v->a_un.a_val & 0x100)) {
          return (unsigned long)getLinkerdlopen(v->a_un.a_val)->libdl_symtab;
        }
        return 0;
      }
    }
    return 0;
  }

  void setauxval(unsigned long type,unsigned long val) {
    for (ElfW(auxv_t)* v = auxv; v->a_type != AT_NULL; ++v) {
      if (v->a_type == type) {
	      v->a_un.a_val = val;
        return ;
      }
    }
    return ;
  }

  int argc;
  char** argv;
  char** envp;
  ElfW(auxv_t)* auxv;


  abort_msg_t** abort_message_ptr;

 private:
  DISALLOW_COPY_AND_ASSIGN(KernelArgumentBlock);
};

#endif // KERNEL_ARGUMENT_BLOCK_H

