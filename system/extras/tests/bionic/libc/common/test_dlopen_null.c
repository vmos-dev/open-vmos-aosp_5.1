#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>

extern int foo(void)
{
    return 42;
}

int (*func_ptr)(void) = foo;

int main(void)
{
    void*  lib = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    void*  symbol;

#if 0
    /* The Gold linker will garbage-collect unused global functions
     * even if --Wl,--export-dynamic is used. So use a dummy global
     * variable reference here to prevent this.
     */
    if (foo() != 42)
        return 3;
#endif

    if (lib == NULL) {
        fprintf(stderr, "Could not open self-executable with dlopen(NULL) !!: %s\n", dlerror());
        return 1;
    }
    symbol = dlsym(lib, "foo");
    if (symbol == NULL) {
        fprintf(stderr, "Could not lookup symbol inside executable !!: %s\n", dlerror());
        return 2;
    }
    dlclose(lib);
    return 0;
}
