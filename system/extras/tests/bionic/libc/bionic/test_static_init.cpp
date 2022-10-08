#include <stdlib.h>
#include <stdio.h>
#include "lib_static_init.h"

Foo  theFoo2;

int  main(int  argc, char**  argv)
{
    int  c = theFoo.getValue();

    /* check the counter on the library object
     * it must have been called first, and only once
     */
    if (c != 1) {
        printf("KO (counter(shared) == %d, expected 1)\n", c);
        return 1;
    }

    /* check the counter on the executable object,
     * it must have been called second, and only once
     */
    c = theFoo2.getValue();
    if (c != 2) {
        printf("KO (counter(executable) == %d, expected 2)\n", c);
        return 1;
    }

    printf("OK\n");
    return 0;
}
