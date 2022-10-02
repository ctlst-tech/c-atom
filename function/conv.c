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

#define CONV_INT_TEMPLATE(__type, __postfix)                 \
CONV_INT_TEMPLATE_PROTO(__type, __postfix) {   \
    *v = strtol(s, NULL, 0);                                 \
    if (errno != 0) {                                        \
        return conv_rv_range;                                \
    }                                                        \
    return conv_rv_ok;                                       \
}

CONV_INT_TEMPLATE(uint64_t, uint64)
CONV_INT_TEMPLATE(int64_t, int64)
CONV_INT_TEMPLATE(uint32_t, uint32)
CONV_INT_TEMPLATE(int32_t, int32)
CONV_INT_TEMPLATE(uint16_t, uint16)
CONV_INT_TEMPLATE(int16_t, int16)
CONV_INT_TEMPLATE(uint8_t, uint8)
CONV_INT_TEMPLATE(int8_t, int8)

conv_rv_t conv_str_bool(const char *s, int32_t *v) {

    if (strcasecmp(s, "true") == 0) {
        *v = -1;
    } else if (strcasecmp(s, "false") == 0) {
        *v = 0;
    } else {
        return conv_rv_format;
    }

    return conv_rv_ok;
}

conv_rv_t conv_str_not_supported(const char *s, void *v) {
    return conv_rv_ok;
}
