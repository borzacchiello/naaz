#include <unistd.h>

int main(int argc, char const* argv[])
{
    float a;
    read(0, &a, sizeof a);

    if (a == 3.14f)
        return 1;
    return 0;
}
