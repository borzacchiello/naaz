#include "../lifter/PCodeLifter.hpp"

int main(int argc, const char* argv[])
{
    naaz::lifter::PCodeLifter lifter(naaz::arch::x86_64::The());

    const uint8_t code[] =
        "\x48\xC7\xC0\x0A\x00\x00\x00\x48\xC7\xC3\x0A\x00\x00"
        "\x00\x48\x39\xD8\x74\x00\xC3";
    size_t code_size = sizeof(code);

    lifter.lift(0x400000, code, code_size)->pp();
    return 0;
}
