/* this is part of the test_relocs.c test, which is used to check that
 * the relocations generated in a shared object are properly handled
 * by the Bionic dynamic linker
 */

struct foo { int first, second; };
struct foo Foo = {1, 2};

int* FooPtr[] = { &Foo.first, &Foo.second };

int func1( void )
{
    return *FooPtr[0];
}

int  func2( void )
{
    return *FooPtr[1];
}
