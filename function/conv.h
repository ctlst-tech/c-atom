#ifndef CATOM_CONV_H
#define CATOM_CONV_H

typedef enum conv_rv {
    conv_rv_ok = 0,
    conv_rv_format,
    conv_rv_range,
    conv_rv_size
} conv_rv_t;

extern conv_rv_t conv_str_double(const char *s, double *v);

#define CONV_INT_TEMPLATE_PROTO(__type, __postfix) \
conv_rv_t conv_str_##__postfix(const char *s, __type *v)

CONV_INT_TEMPLATE_PROTO(uint64_t, uint64);
CONV_INT_TEMPLATE_PROTO(int64_t, int64);
CONV_INT_TEMPLATE_PROTO(uint32_t, uint32);
CONV_INT_TEMPLATE_PROTO(int32_t, int32);
CONV_INT_TEMPLATE_PROTO(uint16_t, uint16);
CONV_INT_TEMPLATE_PROTO(int16_t, int16);
CONV_INT_TEMPLATE_PROTO(uint8_t, uint8);
CONV_INT_TEMPLATE_PROTO(int8_t, int8);

conv_rv_t conv_str_bool(const char *s, int32_t *v);

conv_rv_t conv_str_not_supported(const char *s, void *v);

#endif //CATOM_CONV_H
