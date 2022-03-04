#include <cstdio>

#include "../loader/BFDLoader.hpp"

static void usage(char const* name)
{
    fprintf(stderr, "USAGE: %s <bin>\n", name);
    exit(1);
}

int main(int argc, char const* argv[])
{
    if (argc < 2)
        usage(argv[0]);

    naaz::loader::BFDLoader loader(argv[1]);

    auto  as   = loader.address_space();
    auto& arch = loader.arch();

    printf("%s\n", argv[1]);
    printf("  arch:  %s\n", arch.description().c_str());
    printf("  entry: %08lxh\n", loader.entrypoint());

    printf("\nSegments\n");
    printf("----------------------------------------------------\n");
    printf("  address range                          name       \n");
    printf("----------------------------------------------------\n");
    for (auto& sec : as->segments()) {
        printf("  %016lxh - %016lxh  %s\n", sec.addr(), sec.addr() + sec.size(),
               sec.name().c_str());
    }

    return 0;
}
