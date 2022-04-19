#include <stdlib.h>

int foo(int a)
{
    if (a * a / 16 + 33 == 223)
        return 1;
    return 0;
}

int main(int argc, char const* argv[])
{
    if (argc != 2)
        return -1;

    return foo(atoi(argv[1]));
}
