#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const* argv[])
{
    int i;
    for (i = 0; i < 1000; ++i)
        printf("%d\n", rand());
    return 0;
}
