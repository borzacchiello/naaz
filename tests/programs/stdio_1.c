#include <stdio.h>
#include <unistd.h>

const char* a = "mele";
const char* b = "pere";

int main(int argc, char const* argv[])
{
    int inp;
    read(0, &inp, sizeof inp);

    const char* w = inp ? a : b;
    printf("ciao!\n");
    printf("mi piacciono le %s!\n", w);
    return 0;
}
