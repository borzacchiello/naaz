#include <unistd.h>

int buf[8];

int main(int argc, char const* argv[])
{
    read(0, &buf, sizeof buf);

    unsigned char res = 0;
    int           i;
    for (i = 0; i < sizeof(buf) / sizeof(int); ++i)
        if (buf[i] != 0xaa)
            res += 1;
        else
            res += 2;


    if (res == 16)
        return 1;
    return 0;
}
