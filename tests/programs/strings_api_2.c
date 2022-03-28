#include <string.h>

int main(int argc, char const* argv[])
{
    if (argc < 2)
        return -1;

    if (strlen(argv[1]) == 15)
        return 1;
    return 0;
}
