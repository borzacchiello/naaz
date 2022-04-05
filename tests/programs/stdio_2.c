#include <string.h>
#include <stdio.h>

int main(int argc, char const* argv[])
{
    char str[15] = {0};
    scanf("%14s", str);

    if (memcmp(str, "patatealforno", 14) == 0)
        return 1;
    return 0;
}
