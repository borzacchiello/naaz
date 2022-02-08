#include "../lifter/PCodeLifter.hpp"

int main(int argc, const char* argv[])
{
    naaz::loader::AddressSpace addr_space;
    addr_space.register_segment(
        0x400000,
        (const uint8_t*)"\x48\xC7\xC0\x0A\x00\x00\x00\x48\xC7\xC3\x0A\x00\x00"
                        "\x00\x48\x39\xD8\x74\x00\xC3",
        21, 7);

    naaz::lifter::PCodeLifter lifter(naaz::arch::x86_64::The(), addr_space);
    lifter.lift(0x400000).pp();

    return 0;
}
