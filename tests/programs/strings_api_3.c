#include <string.h>

int main(int argc, char const* argv[])
{
    if (argc < 2)
        return -1;

    char str[128];
    str[0] = 0;

    char* p_str = (char*)str;
    strcat(p_str, "pollo");
    strcat(p_str, "/");
    strcat(p_str, "brodo");

    if (strcmp(argv[1], p_str) == 0)
        return 1;
    return 0;
}
