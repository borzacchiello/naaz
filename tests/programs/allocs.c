#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char buf[128];

int main(int argc, char const* argv[])
{
    read(0, &buf, sizeof buf);

    char* heap_buf1 = (char*)malloc(sizeof buf);
    memcpy(heap_buf1, buf, sizeof buf);

    if (heap_buf1[16] != buf[16])
        // unreachable
        return 1;

    char* heap_buf2 = (char*)realloc(heap_buf1, sizeof buf + sizeof buf);

    if (heap_buf2[16] != buf[16])
        // unreachable
        return 2;

    char* heap_buf3 = (char*)calloc(sizeof buf, sizeof(char));

    if (heap_buf3[16] != 0)
        // unreachable
        return 3;

    free(heap_buf2);
    free(heap_buf3);
    return 0;
}
