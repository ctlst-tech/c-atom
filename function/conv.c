#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "conv.h"

static int contains_only(const char *s, const char *cs) {
    for (; *s != 0; s++) {
        if (!strchr(cs, *s)) {
            return 0;
        }
    }
    return 1;
}

conv_rv_t conv_str_double(const char *s, double *v) {
    double tmp;
    tmp = strtod(s, NULL);

    if (errno != 0) {
        return conv_rv_range;
    }

    if (tmp == 0.0) {
        if (!contains_only(s, "+-0.")) {
            return conv_rv_format;
        }
    }
    *v = tmp;
    return conv_rv_ok;
}