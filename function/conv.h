#ifndef CATOM_CONV_H
#define CATOM_CONV_H

typedef enum conv_rv {
    conv_rv_ok = 0,
    conv_rv_format,
    conv_rv_range
} conv_rv_t;

extern conv_rv_t conv_str_double(const char *s, double *v);

#endif //CATOM_CONV_H
