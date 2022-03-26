#include <unistd.h>

int baz() { return 0; }

int bar() { return 1; }

int foo(int, void*, void*);
__asm__(".globl foo\n"
        "foo:\n"
        "test   %edi, %edi\n"
        "cmovzq %rsi, %rdx\n"
        "call   *%rdx\n"
        "ret\n");

int main(int argc, char const* argv[])
{
    int a;
    read(0, &a, sizeof a);

    return foo(a, &bar, &baz);
}
