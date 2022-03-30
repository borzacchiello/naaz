#include <unistd.h>

int main(int argc, char const* argv[])
{
    int a;
    read(0, &a, sizeof a);
    float aa = (float)a;

    aa = aa * 2.0f + 10.0f;
    if (aa == 4242.0f)
        return 1;
    return 0;
}
