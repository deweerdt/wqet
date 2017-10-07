#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mruby/compile.h"
#include "mruby/data.h"
#include "mruby/error.h"
#include "mruby/hash.h"
#include "mruby/string.h"

static mrb_value wqet_mruby_kernel_sleep(mrb_state *mrb, mrb_value self)
{
    time_t beg, end;
    mrb_value *argv;
    mrb_int argc;
    int iargc;

    beg = time(0);
    mrb_get_args(mrb, "*", &argv, &argc);

    iargc = (int)argc;

    /* not implemented forever sleep (called without an argument)*/
    if (iargc == 0 || iargc >= 2) {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "wrong number of arguments");
    }

    if (mrb_fixnum_p(argv[0]) && mrb_fixnum(argv[0]) >= 0) {
        sleep(mrb_fixnum(argv[0]));
    } else {
        mrb_raise(mrb, E_ARGUMENT_ERROR, "time interval must be positive");
    }
    end = time(0) - beg;

    return mrb_fixnum_value(end);
}


void wqet_run_mruby(const char *rbfile, int argc, char **argv)
{
    mrb_value ARGV;
    int i;
    mrb_state *mrb = mrb_open();

    ARGV = mrb_ary_new_capa(mrb, argc);
    for (i = 0; i < argc; i++) {
        char *utf8 = mrb_utf8_from_locale(argv[i], -1);
        if (utf8) {
            mrb_ary_push(mrb, ARGV, mrb_str_new_cstr(mrb, utf8));
            mrb_utf8_free(utf8);
        }
    }
    mrb_define_global_const(mrb, "ARGV", ARGV);
    struct RClass *h2get_mruby = mrb_define_class(mrb, "H2", mrb->object_class);
    MRB_SET_INSTANCE_TT(h2get_mruby, MRB_TT_DATA);

    /* Kernel */
    mrb_define_method(mrb, mrb->kernel_module, "sleep", wqet_mruby_kernel_sleep, MRB_ARGS_ARG(1, 0));

    FILE *f = fopen(rbfile, "r");
    if (!f) {
        printf("Failed to open file `%s`: %s\n", rbfile, strerror(errno));
        exit(EXIT_FAILURE);
    }
    mrb_value obj = mrb_load_file(mrb, f);
    fclose(f);
    fflush(stdout);
    fflush(stderr);

    if (mrb->exc) {
        obj = mrb_funcall(mrb, mrb_obj_value(mrb->exc), "inspect", 0);
        fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
        putc('\n', stdout);
        exit(EXIT_FAILURE);
    }

    mrb_close(mrb);
    return;
}
