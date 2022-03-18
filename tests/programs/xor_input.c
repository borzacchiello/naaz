#include <unistd.h>

int buf[64];

int main(int argc, char const* argv[])
{
    read(0, &buf, sizeof buf);

    int chk = 0, i = 0;
    for (; i < sizeof(buf) / sizeof(int); ++i)
        chk ^= buf[i];

    if (chk == 0xdeadbeef)
        return 1;
    return 0;
}
