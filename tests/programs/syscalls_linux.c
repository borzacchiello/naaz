#include <stdio.h>

int read_buf(char* buf, int size);
asm(".globl read_buf\n"
    "read_buf:\n"
    "      movq $0,   %rax\n" // read syscall
    "      movq %rsi, %rdx\n" // size
    "      movq %rdi, %rsi\n" // buf
    "      movq $0,   %rdi\n" // stdin fd
    "      syscall\n"
    "      ret\n");

int main(int argc, char const* argv[])
{
    char buf[30];
    read_buf(buf, sizeof buf);

    int chk = 0, i;
    for (i = 0; i < sizeof buf; ++i)
        chk += buf[i];

    if (chk == 13)
        return 1;
    return 0;
}
