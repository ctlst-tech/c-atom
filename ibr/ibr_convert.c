#include <string.h>
#include "ibr_convert.h"




/*
ibr_rv_t ibr_cast_scalar(void *src, field_scalar_type_t st, void *dst, field_scalar_type_t dt) {

}
*/

//{ (*((to__*)dst)) = (*((from__*)(src))); }


// TODO rename CAST to CONV

#define CAST_UNALIGNED(from__,to__) {*dst = *src;}
#define CAST_ALIGNED(__from,__to) {__to##_t t = *src; memcpy(dst, &t, sizeof(t));}

//#define CHNDL CAST_ALIGNED
#define CHNDL CAST_UNALIGNED

#define CAST_F_PREF static void
#define CAST_F_NAME(from__,to__) cast_##from__##_##to__

#define double_t double
#define float_t float

#define CAST_GEN_CELL(from__,to__) CAST_F_PREF CAST_F_NAME(from__,to__)(const from__##_t *src, to__##_t *dst) {CHNDL(from__, to__);}

#define CAST_GEN_COLUMN(from__) \
    CAST_GEN_CELL(from__,uint8) \
    CAST_GEN_CELL(from__,uint16) \
    CAST_GEN_CELL(from__,uint32) \
    CAST_GEN_CELL(from__,uint64) \
    CAST_GEN_CELL(from__,int8) \
    CAST_GEN_CELL(from__,int16) \
    CAST_GEN_CELL(from__,int32) \
    CAST_GEN_CELL(from__,int64) \
    CAST_GEN_CELL(from__,float) \
    CAST_GEN_CELL(from__,double)

CAST_GEN_COLUMN(uint8)
CAST_GEN_COLUMN(uint16)
CAST_GEN_COLUMN(uint32)
CAST_GEN_COLUMN(uint64)
CAST_GEN_COLUMN(int8)
CAST_GEN_COLUMN(int16)
CAST_GEN_COLUMN(int32)
CAST_GEN_COLUMN(int64)
CAST_GEN_COLUMN(float)
CAST_GEN_COLUMN(double)

#define RULE(f__,t__) \
    {.from = ft_##f__, .to = ft_##t__, .h = (conv_handler_t)CAST_F_NAME(f__,t__)}

#define RULE_COLUMN(f__) \
    RULE(f__,uint8), \
    RULE(f__,uint16), \
    RULE(f__,uint32), \
    RULE(f__,uint64), \
    RULE(f__,int8), \
    RULE(f__,int16), \
    RULE(f__,int32), \
    RULE(f__,int64), \
    RULE(f__,float), \
    RULE(f__,double)

static struct conv_operation {
    field_scalar_type_t from;
    field_scalar_type_t to;
    conv_handler_t      h;
} conv_operations[] = {
        RULE_COLUMN(uint8),
        RULE_COLUMN(uint16),
        RULE_COLUMN(uint32),
        RULE_COLUMN(uint64),
        RULE_COLUMN(int8),
        RULE_COLUMN(int16),
        RULE_COLUMN(int32),
        RULE_COLUMN(int64),
        RULE_COLUMN(float),
        RULE_COLUMN(double),
        {.h = NULL}
};




static struct conv_operation *conv_rule_find(field_scalar_type_t st, field_scalar_type_t dt) {

    // TODO plain copy instructions

    for (int i = 0; conv_operations[i].h != NULL; i++) {
        if ((conv_operations[i].from == st) && (conv_operations[i].to == dt)) {
            return &conv_operations[i];
        }
    }

    return NULL;
}

static ibr_rv_t add_instr(conv_instr_queue_t *q, void *src, void *dst, struct conv_rule *rule) {
    if (q->instr_num >= MDM_CONV_MAX_INSTR) {
        return ibr_nomem;
    }

    q->instrs[q->instr_num].dst = dst;
    q->instrs[q->instr_num].src = src;
//    q->instrs[q->instr_num].h = rule->h;

    q->instr_num++;

    return ibr_ok;
}

/*
ibr_rv_t conv_instr_queue_add(conv_instr_queue_t *q, void *src, field_scalar_type_t st, void *dst, field_scalar_type_t dt) {
    struct conv_operation *opr = conv_rule_find(st, dt);

    if (opr == NULL) {
        return ibr_noent;
    }

    return add_instr(q, src, dst, opr);
}

ibr_rv_t conv_instr_queue_init(conv_instr_queue_t *q) {
    memset(q, 0, sizeof (*q));

    return ibr_ok;
}

*/
ibr_rv_t conv_exec(conv_instr_queue_t *q, void *src, int ss, void *dst, int ds) {

    // FIXME
    memcpy(dst, src, ss < ds ? ss : ds);

    return ibr_ok;
}
