#include <unistd.h>

int g_buf[16];

int main(int argc, char const* argv[])
{
    unsigned char i1, i2;
    read(0, &i1, sizeof i1);
    read(0, &i2, sizeof i2);

    g_buf[i1 & 0xf] = 42;
    if (g_buf[i2 & 0xf])
        return 1;
    return 0;
}
