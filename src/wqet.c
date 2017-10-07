#include "wqet.h"
#include "wqet/wqet_mruby.h"


static void usage(void)
{
    fprintf(stderr, "Usage: wqet [filename.rb]>\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int extra_args;

    signal(SIGPIPE, SIG_IGN);

    if (argc < 1) {
        usage();
    }
    extra_args = (argc - 2) >= 0;
    wqet_run_mruby(argv[1], extra_args ? argc - 2 : 0, extra_args ? &argv[2] : NULL);

    exit(EXIT_SUCCESS);
}
