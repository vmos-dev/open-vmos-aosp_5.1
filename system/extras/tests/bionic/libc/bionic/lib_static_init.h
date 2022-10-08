#ifndef _lib_static_init_h
#define _lib_static_init_h

class Foo {
private:
    int         value;
    static int  counter;
public:
    virtual int getValue();
    Foo();
    virtual ~Foo();
};

Foo::~Foo()
{
}

extern Foo  theFoo;

#endif /* _lib_static_init_h */
