#ifndef HW_BRIDGE_MDM_CONVERT_H
#define HW_BRIDGE_MDM_CONVERT_H

#include <eswb/types.h>

#include "ibr.h"
#include "ibr_msg.h"


typedef void (*conv_handler_t)(const void *src, void *dst, const void *opt);

typedef struct {
    unsigned d_src;
    unsigned d_dst;
    double scale;
    double *pscale;
    conv_handler_t h;
} conv_instr_t;

#define MDM_CONV_MAX_INSTR 40

typedef struct conv_instr_queue {
    conv_instr_t instrs [MDM_CONV_MAX_INSTR];
    int instr_num;
} conv_instr_queue_t;

typedef struct conv_rule {
    enum {
        cr_asis = 0,
        cr_omit,
        cr_copy,
        cr_conv,
    } type;

    eswb_type_t bus_type;
    const char *new_name;
    double k;
    double b;
} conv_rule_t;

ibr_rv_t conv_instr_queue_add(conv_instr_queue_t *q, field_scalar_type_t st, field_scalar_type_t dt);

ibr_rv_t conv_instr_queue_init(conv_instr_queue_t *q);
ibr_rv_t conv_exec(conv_instr_queue_t *q, void *src, int ss, void *dst, int ds);

#endif //HW_BRIDGE_MDM_CONVERT_H
