#ifndef __BIONIC_HOOK_H
#define __BIONIC_HOOK_H

#include "pthread.h"
#include <sched.h>

struct addrinfo;
struct sockaddr;
struct __pthread_cleanup_t;

#if defined(__cplusplus)
extern "C" {
#endif

extern int host_faccessat(int dirfd, const char* pathname, int mode);
extern const char* get_abs_path(char* path,int buf_len,char* outbuf,int* outlen);
extern int host_openat(int a1, const char* a2, int a3, mode_t a4);
extern void *get_host_function(int num);
extern int get_proc_uid_gid(pid_t pid, uid_t *uid, gid_t *gid);
extern void* vmosdlopen(const char* filename, int flags);
extern int vmosdlclose(void* handle);
extern void* vmosdlsym(void* handle, const char* symbol);
extern const char* vmosdlerror();
extern void set_use_memfd(int use);
extern int get_use_memfd();
extern int ASharedMemory_create(const char *name, size_t size);
extern size_t ASharedMemory_getSize(int fd);
extern int ASharedMemory_setProt(int fd, int prot);
extern int ASharedMemory_create_from_socket(int *fds);

extern int clone_hook(int (*fn)(void*), void* child_stack, int flags, void* arg, int* parent_tid, void* new_tls, int* child_tid);
extern pid_t getpid_hook();
extern pid_t gettid_hook();
extern void __bionic_atfork_run_prepare_hook();
extern void __bionic_atfork_run_child_hook();
extern void __bionic_atfork_run_parent_hook();
extern void pthread_atfork_hook(void (*prepare)(void), void (*parent)(void), void(*child)(void));
extern int __register_atfork_hook(void (*prepare)(void), void (*parent)(void),void (*child)(void), void* dso);
extern void __unregister_atfork_hook(void* dso);
extern int pthread_getattr_np_hook(pthread_t t, pthread_attr_t* attr);
extern int pthread_create_hook(pthread_t* thread_out, pthread_attr_t const* attr, void* (*start_routine)(void*), void* arg);
extern int pthread_detach_hook(pthread_t t);
extern void __pthread_cleanup_push_hook(__pthread_cleanup_t* c, __pthread_cleanup_func_t routine, void* arg);
extern void __pthread_cleanup_pop_hook(__pthread_cleanup_t* c, int execute);
extern void pthread_exit_hook(void* return_value);
extern int pthread_getcpuclockid_hook(pthread_t t, clockid_t* clockid);
extern pid_t pthread_gettid_np_hook(pthread_t t);
extern int pthread_join_hook(pthread_t t, void** return_value);
extern void pthread_key_clean_all_hook();
extern int pthread_key_create_hook(pthread_key_t* key, void (*key_destructor)(void*));
extern int pthread_key_delete_hook(pthread_key_t key);
extern void* pthread_getspecific_hook(pthread_key_t key);
extern int pthread_setspecific_hook(pthread_key_t key, const void* ptr);
extern int pthread_kill_hook(pthread_t t, int sig);
extern int pthread_getschedparam_hook(pthread_t t, int* policy, void* /*sched_param*/ param);
extern int pthread_setname_np_hook(pthread_t t, const char* thread_name);
extern int pthread_setschedparam_hook(pthread_t t, int policy, const void* /*sched_param*/ param);
extern int getaddrinfo_hook(const char *hostname, const char *servname,const struct addrinfo *hints, struct addrinfo **res);
extern struct hostent* gethostbyname_hook(const char *name);
extern struct hostent* gethostbyname2_hook(const char *name, int af);
extern struct hostent* gethostbyaddr_hook(const void *addr, socklen_t len, int af);
extern int getnameinfo_hook(const struct sockaddr* sa, socklen_t salen, char* host, size_t hostlen, char* serv, size_t servlen, int flags);
extern __noreturn void exit_hook(int status);
extern void registerVmGetPropFunc(void* func);

extern void* mhkcalloc(size_t n_elements, size_t elem_size);
extern void mhkfree(void* mem);
extern struct mallinfo mhkmallinfo();
extern void* mhkmalloc(size_t bytes);
extern size_t mhkmalloc_usable_size(const void* mem);
extern void* mhkmemalign(size_t alignment, size_t bytes);
extern int mhkposix_memalign(void** memptr, size_t alignment, size_t size);
extern void* mhkpvalloc(size_t bytes);
extern void* mhkrealloc(void* oldMem, size_t bytes);
extern void* mhkvalloc(size_t bytes);

#if defined(__cplusplus)
}
#endif
#endif /* __BIONIC_HOOK_H */