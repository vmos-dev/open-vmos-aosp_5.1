#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void printf_log(const char *fmt, ...)
{
    va_list lst;
    va_start(lst, fmt);
    vprintf(fmt, lst);
    va_end(lst);
}

/* Override this for non-printf reporting */
extern void (*malloc_log)(const char *fmt, ...);
static void ctor(void) __attribute__((constructor));
static void ctor(void)
{
    malloc_log = printf_log;
}

int main(void)
{
	char *ptr[6];
	char *uaf;
	char *cf, *cb;

	ptr[0] = malloc(10);
	ptr[1] = calloc(1,20);
	ptr[2] = malloc(30);
	ptr[3] = malloc(40);
        ptr[4] = malloc(50);
        ptr[5] = malloc(60);

	free(ptr[1]);
	free(ptr[1]);
	free(ptr[2]);
        ptr[2] = realloc(ptr[2], 300);
//      free(ptr[2]);
//      free(ptr[2]);

	uaf = ptr[3];
	free(uaf);
	uaf[5] = 'a';

        cf = ptr[4];
        cf[-1] = 'a'; 

        cb = ptr[5];
        cb[60] = 'a';

	sleep(10);

	return 0;
}
