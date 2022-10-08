/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * libc_init_dynamic.c
 *
 * This source files provides two important functions for dynamic
 * executables:
 *
 * - a C runtime initializer (__libc_preinit), which is called by
 *   the dynamic linker when libc.so is loaded. This happens before
 *   any other initializer (e.g. static C++ constructors in other
 *   shared libraries the program depends on).
 *
 * - a program launch function (__libc_init), which is called after
 *   all dynamic linking has been performed. Technically, it is called
 *   from arch-$ARCH/bionic/crtbegin_dynamic.S which is itself called
 *   by the dynamic linker after all libraries have been loaded and
 *   initialized.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include "libc_init_common.h"
#include "hkmalloc.h"

#include "pthread_key_hook.h"

#include "private/bionic_tls.h"
#include "private/KernelArgumentBlock.h"
#include "private/libc_logging.h"
extern "C" {
  extern void malloc_debug_init(void);
  extern void malloc_debug_fini(void);
  extern void netdClientInit(void);
  extern int __cxa_atexit(void (*)(void *), void *, void *);
  int getresuid(uid_t* a1, uid_t* a2, uid_t* a3);

  int getresgid(gid_t* a1, gid_t* a2, gid_t* a3) ;
  uid_t geteuid();
  gid_t getegid();
};
void init_pthread_(KernelArgumentBlock* args);

// struct syscall_name_t {
//   const char* name;
//   int syscall_no;
// };
// syscall_name_t syscall_name[] = {
//   #include "libc_util.h"
// };

// const char* get_syscall_name(int syscall_no) {
//   size_t size = sizeof(syscall_name) / sizeof(syscall_name[0]);
//   for (int i = 0; i < size; i++) {
//     if(syscall_no == syscall_name[i].syscall_no)
//       return syscall_name[i].name;
//   }
//   return "invalid syscall_no";
// }

#include "private/kernel_sigset_t.h"
#include <sys/prctl.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <asm/unistd.h>
extern "C" int __rt_sigprocmask(int, const kernel_sigset_t*, kernel_sigset_t*, size_t);
extern "C" int __sigaction(int, const struct sigaction*, struct sigaction*);
extern "C" long _syscall(long __number, ...);
static int get_si_syscall(const siginfo_t *info)
{
#if defined(si_syscall)
	return info->si_syscall;
#endif

	typedef struct {
		void		*ip;
		int		nr;
		unsigned int	arch;
	} local_siginfo_t;

	union {
		const siginfo_t *info;
		const local_siginfo_t *local_info;
	} local_info = {
		.info = info,
	};
	return local_info.local_info->nr;
}
#include <stdio.h>
#include "linux/capability.h"   // rm-root include capset capget
#include <dirent.h>
extern "C" char* getRootfsPath();
extern "C" int __openat(int fd, const char* name, int flags, int mode);
extern "C" int __connect(int a, const struct sockaddr* sck, socklen_t c);
extern "C" int faccessat(int dirfd, const char *path, int mode, int flags );
extern "C" long syscall(long number, ...);
extern "C" int unlinkat(int dirfd, const char *pathname, int flags);
extern "C" int execve(const char *filename, char * const *argv, char * const *envp);
extern "C" ssize_t readlinkat(int fd, const char* name, char* buf, size_t size);
extern "C" int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
extern "C" uid_t getuid();
extern "C" gid_t getgid();
extern "C" int fstatat64(int dirfd, const char *path, struct stat64 * buf , int flags );
extern "C" int mkdirat(int fd, const char *pathname, mode_t mode);
extern "C" int __getdents64(unsigned int dirfd, struct dirent* buf, unsigned int bufsize);

static void log_sigsys_handler(int sig, siginfo_t *info,
			void *context)
{
  (void)sig;
	ucontext* pucontext = (ucontext*)context;
	// const char *syscall_name;
	int nr = get_si_syscall(info);
	  // // __libc_fatal_no_abort("blocked syscall no: %d %s pc: %zx", nr, get_syscall_name(nr),pucontext->uc_mcontext.arm_pc);
    // // __libc_fatal_no_abort("blocked syscall context: r0:%zx r1:%zx r2:%zx r3:%zx r4:%zx r5:%zx r6:%zx", pucontext->uc_mcontext.arm_r0, pucontext->uc_mcontext.arm_r1, pucontext->uc_mcontext.arm_r2, pucontext->uc_mcontext.arm_r3, pucontext->uc_mcontext.arm_r4, pucontext->uc_mcontext.arm_r5, pucontext->uc_mcontext.arm_r6);
	// _syscall(__NR_getcwd);
	intptr_t ret = 0;
	// if (nr == __NR_sigaction)
	// 	ret = 0;
	// else 
	// 	ret = _syscall(nr, pucontext->uc_mcontext.arm_r0, pucontext->uc_mcontext.arm_r1, pucontext->uc_mcontext.arm_r2, pucontext->uc_mcontext.arm_r3, pucontext->uc_mcontext.arm_r4, pucontext->uc_mcontext.arm_r5, pucontext->uc_mcontext.arm_r6);
#if defined(__LP64__)
	  // __libc_fatal_no_abort("blocked syscall no: %d %s pc: %zx", nr, get_syscall_name(nr), pucontext->uc_mcontext.pc);
    // // __libc_fatal_no_abort("blocked syscall context: r0:%zx r1:%zx r2:%zx r3:%zx r4:%zx r5:%zx r6:%zx", pucontext->uc_mcontext.regs[0], pucontext->uc_mcontext.regs[1], pucontext->uc_mcontext.regs[2], pucontext->uc_mcontext.regs[3], pucontext->uc_mcontext.regs[4], pucontext->uc_mcontext.regs[5], pucontext->uc_mcontext.regs[6]);
	switch(nr) {
    case __NR_rt_sigaction: {
      ret = 0;
      break;
    }
    default: {
      ret = syscall(nr, pucontext->uc_mcontext.regs[0], pucontext->uc_mcontext.regs[1], pucontext->uc_mcontext.regs[2], pucontext->uc_mcontext.regs[3], pucontext->uc_mcontext.regs[4], pucontext->uc_mcontext.regs[5], pucontext->uc_mcontext.regs[6]);
    }
	}
  //LOGE("syscall ret: %d", ret);
  pucontext->uc_mcontext.regs[0] = ret == -1 ? -errno : ret;
#else
	  // // __libc_fatal_no_abort("blocked syscall no: %d %s pc: %zx", nr, get_syscall_name(nr),pucontext->uc_mcontext.arm_pc);
    // // __libc_fatal_no_abort("blocked syscall context: r0:%zx r1:%zx r2:%zx r3:%zx r4:%zx r5:%zx r6:%zx", pucontext->uc_mcontext.arm_r0, pucontext->uc_mcontext.arm_r1, pucontext->uc_mcontext.arm_r2, pucontext->uc_mcontext.arm_r3, pucontext->uc_mcontext.arm_r4, pucontext->uc_mcontext.arm_r5, pucontext->uc_mcontext.arm_r6);
	switch(nr) {
    case __NR_ioprio_set:
    case __NR_rt_sigaction:
    case __NR_sigaction: {
      ret = 0;
      break;
    }
		default: {
      ret = syscall(nr, pucontext->uc_mcontext.arm_r0, pucontext->uc_mcontext.arm_r1, pucontext->uc_mcontext.arm_r2, pucontext->uc_mcontext.arm_r3, pucontext->uc_mcontext.arm_r4, pucontext->uc_mcontext.arm_r5, pucontext->uc_mcontext.arm_r6);
      break;
    }
	}

  //LOGE("syscall ret: %d", ret);
  pucontext->uc_mcontext.arm_r0 = ret == -1 ? -errno : ret;
#endif
}


int sigaction2(int signal, const struct sigaction* bionic_new_action, struct sigaction* bionic_old_action);
void install_sigsys_handler(void)
{
	int ret = 0;
	struct sigaction act;
	sigset_t mask;

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = &log_sigsys_handler;
	act.sa_flags = SA_SIGINFO;
  memset(&mask, 0, sizeof(sigset_t));
	sigaddset(&mask, SIGSYS);

	ret = sigaction2(SIGSYS, &act, NULL);
	if (ret < 0)
		return ;

	ret = sigprocmask(SIG_UNBLOCK, &mask, NULL);
	if (ret < 0)
		return ;
	return;
}
#include <fcntl.h>
#include <sys/mman.h>
#include <__a_bionic_hook.h>
extern "C" pid_t getpid();
// We flag the __libc_preinit function as a constructor to ensure
// that its address is listed in libc.so's .init_array section.
// This ensures that the function is called by the dynamic linker
// as soon as the shared library is loaded.

void fix_maps() {
  int fd = host_openat(-100, "/proc/self/maps", O_RDONLY, 0);
  FILE* maps = fdopen(fd, "r");
  char line[256] = { 0 };
  uintptr_t  start, end;
  char       perm[5];
  if (NULL != maps) {
    while (fgets(line, sizeof(line), maps)) {
        if(sscanf(line, "%lx-%lx %4s ", &start, &end, perm) != 3) continue;
        if (strncmp(perm, "--xp", sizeof("--xp") - 1) == 0 && end > start) {
          mprotect((const void*)start, (size_t)(end - start), PROT_EXEC | PROT_READ);
        }
    }
    fclose(maps);
  }
}


__attribute__((constructor)) static void __libc_preinit() {
  // Read the kernel argument block pointer from TLS.
  void** tls = __get_tls();
  KernelArgumentBlock** args_slot = &reinterpret_cast<KernelArgumentBlock**>(tls)[TLS_SLOT_BIONIC_PREINIT];
  KernelArgumentBlock* args = *args_slot;

  // Clear the slot so no other initializer sees its value.
  // __libc_init_common() will change the TLS area so the old one won't be accessible anyway.
  *args_slot = NULL;
#ifdef USE_HKMALLOC
    // init_hkmalloc(args);
    init_pthread_(args);
#endif

// TODO: initlinker
  __libc_init_common(*args);
//    init_pthread_key(args);
    // init hk malloc
#if __LP64__
  fix_maps();
#endif
  // Hooks for various libraries to let them know that we're starting up.
  // malloc_debug_init();
  // netdClientInit();
  install_sigsys_handler();
  // if (!is_pid_in_maps(getpid()))
}

__LIBC_HIDDEN__ void __libc_postfini() {
  // A hook for the debug malloc library to let it know that we're shutting down.
  // malloc_debug_fini();
}

// This function is called from the executable's _start entry point
// (see arch-$ARCH/bionic/crtbegin_dynamic.S), which is itself
// called by the dynamic linker after it has loaded all shared
// libraries the executable depends on.
//
// Note that the dynamic linker has also run all constructors in the
// executable at this point.
__noreturn void __libc_init(void* raw_args,
                            void (*onexit)(void) __unused,
                            int (*slingshot)(int, char**, char**),
                            structors_array_t const * const structors) {

  KernelArgumentBlock args(raw_args);

  // Several Linux ABIs don't pass the onexit pointer, and the ones that
  // do never use it.  Therefore, we ignore it.

  // The executable may have its own destructors listed in its .fini_array
  // so we need to ensure that these are called when the program exits
  // normally.
  if (structors->fini_array) {
    __cxa_atexit(__libc_fini,structors->fini_array,NULL);
  }

  exit(slingshot(args.argc, args.argv, args.envp));
}
