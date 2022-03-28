int main(int argc, char const* argv[])
{
    if (argc < 2)
        return -1;

    if (argv[1][0] == 'k')
        return 1;
    return 0;
}
