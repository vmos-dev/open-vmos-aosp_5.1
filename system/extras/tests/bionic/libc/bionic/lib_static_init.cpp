#include "lib_static_init.h"
#include <stdio.h>

Foo::Foo()
{
    /* increment the static variable */
    value = ++Foo::counter;
    fprintf(stderr, "Foo::Foo for this=%p called (counter = %d)\n", this, counter);
}

int Foo::getValue()
{
    return value;
}

int Foo::counter;

Foo  theFoo;
