#include <unistd.h>

int main(int argc, char const* argv[])
{
    int a;
    read(0, &a, sizeof a);

    switch (a) {
        case 0:
            return 42;
        case 1:
            return 123;
        case 2:
            return 2231;
        case 3:
            return 1;
        case 4:
            return 3341;
        default:
            break;
    }
    return 0;
}
