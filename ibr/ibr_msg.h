#ifndef HW_IBR_H
#define HW_IBR_H

#include <stdint.h>
#include "ibr.h"

//#define IBR_MAX_NAME_LNG 20
//#define IBR_MAX_UNIT_LNG 8

typedef enum field_class {
    fc_scalar,
    fc_bitfield,
    fc_enum,
    fc_flag,
    fc_array,
    fc_sub_pkt,  // don't like the name, but it gives closer definition
    fc_plain_data
} field_class_t;


typedef enum field_scalar_type {
    ft_invalid = 0,
    ft_float,
    ft_double,
    ft_uint8,
    ft_int8,
    ft_uint16,
    ft_int16,
    ft_uint32,
    ft_int32,
    ft_uint64,
    ft_int64,
    ft_char

//    ft_string // not quite scalar ...
} field_scalar_type_t;

typedef struct {
    field_class_t           cls;
    field_scalar_type_t     st;
    int                     size;
} field_type_t;

typedef struct enum_variant {
    const char *name;
    int val;
    const char *description;
    struct enum_variant * next;
} enum_variant_t;

#define IBR_FIELD_FLAG_HAS_SCALE_CONV (1 << 0)
#define IBR_FIELD_FLAG_IN_BITFIELD (1 << 1)
#define IBR_FIELD_FLAG_SIZE_IS_RLTV (1 << 2)     /** size is relative to pkt size */
#define IBR_FIELD_HAS_NESTING (1 << 3)

typedef uint32_t field_flags_t;

struct conv_rule;

typedef struct field {
    field_class_t cls;

    // void * ref;
    int size;           // in bytes, for ft_bitfield* nested fields in bits
    int offset;         // in bytes, for ft_bitfield* nested fields in bits
    // may seems redundant, but used for entered info validation

    // range?

    field_flags_t flags;

    const char *name;
    const char *unit;
    const char *description;

    double scale_factor;

    union {
        field_scalar_type_t scalar_type;
        struct msg *data;
        struct msg *bitfield_list;

        struct {
            struct field *element;
            int min_size;
            int max_size;
        } array;

        enum_variant_t *enum_variants_list;
    } nested;

    struct conv_rule *rule;

    struct field * next;
} field_t;


typedef struct msg {
    const char *name;
    const char *description;

    int32_t id; // if  <0 then - invalid

    int size;
    field_t *fields_list_head;
    //field_t *fields_list_tail;
} msg_t;



typedef struct frame {
    const char *name;
    const char *description;

    msg_t *structure;

    uint32_t payload_offset;

    field_t *resolve_id;
    field_t *resolve_len;

    unsigned msg_num;
    msg_t **msgs;
} frame_t;

typedef struct demrsh_handle {
    msg_t *format;
    field_t *varlng_field;

    int head_size;
    int body_offset;
    int tail_size;

    void *head;
    void *body;
    void *tail;
} demrsh_handle_t;

int ibr_msg_fields_num(msg_t *m);
field_t *ibr_msg_field_find(msg_t *m, const char *name);

field_scalar_type_t ibr_get_equivalent_type_for_bitfield_size(unsigned s);

ibr_rv_t ibr_add_scalar(msg_t *d, const char *name, field_scalar_type_t type, int *offset, field_t **r);
ibr_rv_t ibr_add_bitfield(msg_t *d, const char *name, int size_in_bytes, int *offset, field_t **r);
ibr_rv_t ibr_enum_add_variant(field_t *f, const char *name, int val, const char *description);
ibr_rv_t ibr_bitfield_add_element_flag(field_t *f, const char *name, int *offset_int_bits, field_t **r);
ibr_rv_t ibr_bitfield_add_element_enum(field_t *f, const char *name, int size_in_bits, int *offset_int_bits, field_t **r);

ibr_rv_t ibr_add_dummy(msg_t *d, const char *name, int size, int *offset, field_t **r);
ibr_rv_t ibr_add_string(msg_t *d, const char *name, int size, int *offset, field_t **r);
ibr_rv_t ibr_add_array(msg_t *d, const char *name, int elem_size, int array_size, int *offset, field_t **r);

ibr_rv_t ibr_field_scalar_add_scaling(field_t *f, double factor);

struct conv_instr_queue;
ibr_rv_t ibr_msg_to_functional_msg(msg_t *src, msg_t **dst_rv, struct conv_instr_queue *conv_queue);

field_scalar_type_t ibr_scalar_typefromstr(const char *ts);
topic_data_type_t ibr_eswb_field_type(field_type_t *ft);
int ibr_get_scalar_size(field_scalar_type_t t);

#endif //HW_IBR_H
