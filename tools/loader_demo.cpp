#include <cstdio>
#include <cerrno>

#include "../loader/BFDLoader.hpp"

#define MAX_DUMP 8192

static void usage(char const* name)
{
    fprintf(stderr, "USAGE: %s <bin>\n", name);
    exit(1);
}

static const char* uint64_arg(const char* arg, uint64_t* out)
{
    const char* needle = arg;
    while (*needle == ' ' || *needle == '\t')
        needle++;

    char*    res;
    uint64_t num = strtoul(needle, &res, 0);
    if (needle == res)
        return NULL; // no character
    if (errno != 0)
        return NULL; // error while parsing

    *out = num;
    return res;
}

static void dump_hex(naaz::loader::AddressSpace& as, uint64_t off,
                     uint64_t size)
{
    printf("\n");
    printf("            00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    printf("            -----------------------------------------------\n");
    uint64_t addr = off;
    while (addr < off + size) {
        printf("0x%08lx: ", addr);
        uint64_t i = 0;
        while (addr + i < off + size && i < 16) {
            auto val = as.read_byte(addr + i);
            if (val.has_value())
                printf("%02x ", val.value());
            else
                printf("-- ");
            i++;
        }
        printf("\n");
        addr += i;
    }
    printf("\n");
}

static void dump_segments(naaz::loader::AddressSpace& as)
{
    printf("\nSegments\n");
    printf("------------------------------------------------------\n");
    printf("  address range                            name       \n");
    printf("------------------------------------------------------\n");
    for (auto& sec : as.segments()) {
        printf("  0x%016lx - 0x%016lx  %s\n", sec.addr(),
               sec.addr() + sec.size(), sec.name().c_str());
    }
    printf("\n");
}

static void dump_symbols(naaz::loader::AddressSpace& as)
{
    printf("\nSymbols\n");
    printf("------------------------------------------------------\n");
    printf("  address             type          name              \n");
    printf("------------------------------------------------------\n");

    for (auto const& [addr, syms] : as.symbols()) {
        for (auto const& sym : syms) {
            printf("  0x%016lx  %s  %s\n", sym.addr(),
                   naaz::loader::Symbol::type_to_string(sym.type()).c_str(),
                   sym.name().c_str());
        }
    }
    printf("\n");
}

int main(int argc, char const* argv[])
{
    if (argc < 2)
        usage(argv[0]);

    naaz::loader::BFDLoader loader(argv[1]);

    auto& as   = loader.address_space();
    auto& arch = loader.arch();

    printf("Loaded %s (arch %s)\n\n", argv[1], arch.description().c_str());

    static char buf[256];
    uint64_t    off = 0;

    while (1) {
        printf("0x%08lx> ", off);
        char* line = fgets(buf, sizeof(buf), stdin);
        if (!line || strcmp(line, "quit\n") == 0) {
            break;
        } else if (line[0] == 's') {
            const char* needle = uint64_arg(line + 1, &off);
            if (!needle)
                printf("!Err: unknown argument %s", line + 1);
        } else if (line[0] == 'd') {
            uint64_t    size;
            const char* needle = uint64_arg(line + 1, &size);
            if (size > MAX_DUMP) {
                printf("!Err: size %lu is too high\n", size);
                continue;
            }

            dump_hex(as, off, size);
        } else if (line[0] == 'i') {
            switch (line[1]) {
                case 'S':
                    dump_segments(as);
                    break;
                case 's':
                    dump_symbols(as);
                    break;
                default:
                    printf("!Err: unknown command %s", line);
                    break;
            }
        } else if (strcmp(line, "h\n") == 0 || strcmp(line, "help\n") == 0) {
            printf("\n"
                   "s <addr>: seek to address\n"
                   "d <n>:    dump <n> bytes\n"
                   "iS:       print segments\n"
                   "is:       print symbols\n"
                   "h(elp):   print help\n"
                   "\n");
        } else if (strcmp(line, "\n") == 0) {
            continue;
        } else {
            printf("!Err: unknown command %s", line);
        }
    }

    return 0;
}
