#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int buf[128];

int main()
{
    int fd = open("foo.txt", O_RDONLY);
    if (fd < 0)
        return -1;

    read(fd, &buf, sizeof buf);

    int i, chk = 1;
    for (i = 0; i < sizeof(buf) / sizeof(int); ++i)
        chk *= buf[i];

    if (chk == 0xdead)
        return 2;
    return 0;
}
