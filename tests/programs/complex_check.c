#include <unistd.h>

unsigned char buf[128];

int main(int argc, char const *argv[])
{
    read(0, &buf, sizeof buf);

    int i, res = 0;
    for (i=0; i<sizeof buf; ++i)
        res += (buf[i] ^ 0xaa) << (i & 3);

    if (res == 42)
        return 1;
    return 0;
}
