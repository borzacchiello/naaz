#include <unistd.h>

int main(int argc, char const* argv[])
{
    int a;
    read(0, &a, sizeof a);

    float aa = (float)a;
    if (a == 4242.0f)
        return 1;
    return 0;
}
