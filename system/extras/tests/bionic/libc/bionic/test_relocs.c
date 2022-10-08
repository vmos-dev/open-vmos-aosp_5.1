/* this little test is written to check that the relocations generated
 * in a shared library are correct. it depends on the content of lib_relocs.c
 * being compiled as a shared object.
 */
#include <stdio.h>

extern int  func1(void);
extern int  func2(void);

int
main( void )
{
    int   f1, f2, expect1 = 1, expect2 = 2;

    f1 = func1();
    f2 = func2();

    printf( "func1() returns %d: %s\n", f1, (f1 == expect1) ? "OK" : "FAIL" );
    printf( "func2() returns %d: %s\n", f2, (f2 == expect2) ? "OK" : "FAIL" );

    if (f1 != expect1 || f2 != expect2)
        return 1;

    return 0;
}
