#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ibr_msg.h"
#include "ibr_convert.h"

field_t *ibr_alloc_field() {
    return calloc(1, sizeof(field_t));
}

msg_t *ibr_alloc_msg() {
    return calloc(1, sizeof(msg_t));
}

enum_variant_t *ibr_alloc_enum_variant() {
    return calloc(1, sizeof(enum_variant_t));
}

int ibr_msg_fields_num(msg_t *m) {
    int rv = 0;

    for (field_t *n = m->fields_list_head; n != NULL; n = n->next) {
        rv++;
    }

    // TODO extent when will have a deal with nested fields eg bitfields

    return rv;
}

ibr_rv_t ibr_add_field(msg_t *d, const char *name, field_class_t cls, field_flags_t flags, int size,
                       int *offset, field_t **r) { //const char *unit, const char *description
    field_t *new, **tail, *n;

    if (d->fields_list_head != NULL) {
        for (n = d->fields_list_head; n != NULL; n = n->next) {
            if (strcmp(n->name, name) == 0) {
                return ibr_exist;
            }
            tail = &n->next;
        }
    } else {
        tail = &d->fields_list_head;
    }

    new = ibr_alloc_field();
    if (new == NULL) {
        return ibr_nomem;
    }

    new->name = name;
    new->cls = cls;
    new->flags = flags;
    new->size = size;
    new->offset = *offset;

    (*offset) += size;

    *tail = new;

    if (r != NULL) {
        *r = new;
    }

    d->size = *offset; // FIXME might be corrupted any time. Need to calc it in a way of finalizing message

    return ibr_ok;
}

int ibr_get_scalar_size(field_scalar_type_t t) {
    switch (t) {
        case ft_int8:
        case ft_uint8:
            return 1;
        case ft_int16:
        case ft_uint16:
            return 2;
        case ft_int32:
        case ft_uint32:
        case ft_float:
            return 4;
        case ft_int64:
        case ft_uint64:
        case ft_double:
            return 8;

        default:
            return 0;
    }
}

ibr_rv_t ibr_add_scalar(msg_t *d, const char *name, field_scalar_type_t type, int *offset, field_t **r) {
    int size = ibr_get_scalar_size(type);
    if (size == 0) {
        return ibr_invarg;
    }

    field_t *f;
    ibr_rv_t rv = ibr_add_field(d, name, fc_scalar, 0, size, offset, &f);
    if (rv == ibr_ok) {
        f->nested.scalar_type = type;
    }

    if (r != NULL) {
        *r = f;
    }

    return rv;
}

ibr_rv_t ibr_add_dummy(msg_t *d, const char *name, int size, int *offset, field_t **r) {
    return ibr_add_field(d, name, fc_plain_data, 0, size, offset, r);
}

ibr_rv_t ibr_add_array(msg_t *d, const char *name, int elem_size, int array_size, int *offset, field_t **r) {
    return ibr_add_field(d, name, fc_array, 0, elem_size * array_size, offset, r);
}

ibr_rv_t ibr_add_string(msg_t *d, const char *name, int size, int *offset, field_t **r) {
    return ibr_add_array(d, name, 1, size, offset, r);
}

ibr_rv_t ibr_add_bitfield(msg_t *d, const char *name, int size_in_bytes, int *offset, field_t **r) {

    msg_t *new_d = ibr_alloc_msg();
    if (d == NULL) {
        return ibr_nomem;
    }

    ibr_rv_t rv = ibr_add_field(d, name, fc_bitfield, 0, size_in_bytes, offset, r);
    if (rv != ibr_ok) {
        return rv;
    }

    new_d->name = name;
    (*r)->nested.bitfield_list = new_d;
    return ibr_ok;
}

ibr_rv_t ibr_enum_add_variant(field_t *f, const char *name, int val, const char *description) {

    enum_variant_t *n, *new, **tail;

    if (f->nested.enum_variants_list != NULL) {
        for (n = f->nested.enum_variants_list; n->next != NULL; n = n->next) {
            if (n->val == val) {
                return ibr_exist;
            }
        }
        tail = &n->next;
    } else {
        tail = &f->nested.enum_variants_list;
    }

    new = ibr_alloc_enum_variant();
    if (new == NULL) {
        return ibr_nomem;
    }

    new->val = val;

    new->name = name;
    new->description = description;

    *tail = new;

    return ibr_ok;
}

ibr_rv_t ibr_bitfield_add_element_flag(field_t *f, const char *name, int *offset_int_bits, field_t **r) {
    ibr_rv_t rv = ibr_add_field(f->nested.bitfield_list, name, fc_flag, IBR_FIELD_FLAG_IN_BITFIELD, 1, offset_int_bits, r);
    return rv;
}

ibr_rv_t ibr_bitfield_add_element_enum(field_t *f, const char *name, int size_in_bits, int *offset_int_bits, field_t **r) {
    ibr_rv_t rv = ibr_add_field(f->nested.bitfield_list, name, fc_enum, IBR_FIELD_FLAG_IN_BITFIELD, size_in_bits, offset_int_bits, r);
    return rv;
}

ibr_rv_t ibr_field_scalar_add_scaling(field_t *f, double factor) {

    f->flags |= (factor != 0.0 && factor != 1.0) ? IBR_FIELD_FLAG_HAS_SCALE_CONV : 0;
    f->scale_factor = factor;

    return ibr_ok;
}

ibr_rv_t ibr_field_annotate(field_t *f, const char *unit, const char *description) {

    f->unit = unit;
    f->description = description;

    return ibr_ok;
}


static ibr_rv_t copy_nested_bitfields(msg_t *src, field_t *dst_f) {

    int offset_int_bits = 0;
    field_t *new_f;
    ibr_rv_t rv;

    for (field_t *f = src->fields_list_head; f != NULL; f = f->next) {
        switch (f->cls) {
            case fc_flag:
                rv = ibr_bitfield_add_element_flag(dst_f, f->name, &offset_int_bits, &new_f);
                break;

            case fc_enum:
                rv = ibr_bitfield_add_element_enum(dst_f, f->name, f->size, &offset_int_bits, &new_f);
                break;

            default:
                rv = ibr_not_sup;
                break;
        }

        if (rv != ibr_ok) {
            return rv;
        }
    }

    return ibr_ok;
}

ibr_rv_t ibr_msg_to_functional_msg(msg_t *src, msg_t **dst_rv, conv_instr_queue_t *conv_queue) {

    msg_t *dst = ibr_alloc_msg();
    ibr_rv_t rv;
    int offset = 0;
    field_t *new_f;
    field_scalar_type_t dst_ft;
    field_scalar_type_t src_ft;

    for (field_t *f = src->fields_list_head; f != NULL; f = f->next) {
        switch (f->cls) {
            case fc_scalar:
                ;
                // if we have scale factor, then represent destination field as 'double'
                src_ft = dst_ft = f->nested.scalar_type;
                if (f->flags & IBR_FIELD_FLAG_HAS_SCALE_CONV) {
                    dst_ft = ft_double;
                }

                rv = ibr_add_scalar(dst, f->name, dst_ft, &offset, &f);

                break;

            case fc_bitfield:
                // just copy
                rv = ibr_add_bitfield(dst, f->name, f->size, &offset, &new_f);
                if (rv == ibr_ok) {
                    rv = copy_nested_bitfields(f->nested.bitfield_list, new_f);
                }

                switch (f->size) {
                    case 1: src_ft = dst_ft = ft_uint8; break;
                    case 2: src_ft = dst_ft = ft_uint16; break;
                    case 4: src_ft = dst_ft = ft_uint32; break;
                    case 8: src_ft = dst_ft = ft_uint64; break;
                    default:
                        rv = ibr_invarg;
                }
                break;

            default:
                rv = ibr_not_sup;
                break;
        }
        if (rv != ibr_ok) {
            return rv;
        }

        rv = conv_instr_queue_add(conv_queue, src_ft, dst_ft);
        if (rv != ibr_ok) {
            return rv;
        }
    }

    *dst_rv = dst;

    return ibr_ok;
}

const char *scalar_type_caption(field_scalar_type_t t) {
    switch (t) {
        case ft_int8:   return "I8";
        case ft_uint8:  return "U8";
        case ft_int16:  return "I16";
        case ft_uint16: return "U16";
        case ft_int32:  return "I32";
        case ft_uint32: return "U32";
        case ft_int64:  return "I64";
        case ft_uint64: return "U64";
        case ft_float:  return "F";
        case ft_double: return "D";
        case ft_char: return "C";
        default:        return "??";
    }
}

field_scalar_type_t ibr_scalar_typefromstr(const char *ts) {
    if (ts == NULL) {
        return ft_invalid;
    }

    if (strcmp(ts, "int8") == 0 )           return ft_int8;
    else if (strcmp(ts, "uint8") == 0 )     return ft_uint8;
    else if (strcmp(ts, "int16") == 0 )     return ft_int16;
    else if (strcmp(ts, "uint16") == 0 )    return ft_uint16;
    else if (strcmp(ts, "int32") == 0 )     return ft_int32;
    else if (strcmp(ts, "uint32") == 0 )    return ft_uint32;
    else if (strcmp(ts, "int64") == 0 )     return ft_int64;
    else if (strcmp(ts, "uint64") == 0 )    return ft_uint64;
    else if (strcmp(ts, "float") == 0 )     return ft_float;
    else if (strcmp(ts, "double") == 0 )    return ft_double;
    else if (strcmp(ts, "char") == 0 )      return ft_char;
    else return ft_invalid;
}

const char *class_caption(field_class_t c) {
    switch (c) {
        case fc_scalar:     return "scalar";
        case fc_bitfield:   return "bitfield";
        case fc_enum:       return "enum";
        case fc_flag:       return "flag";
        case fc_array:      return "array";
        case fc_sub_pkt:    return "sub_pkt";
        default:            return "!unknown!";
    }
}

static void nt(int spaces_num) {
    for (int i = 0; i < spaces_num; i++)
        putchar(' ');
}

#define SPL 2 /*space per level*/

static void tb(int nesting) {
    nt(nesting * SPL);
}

void print_enum(enum_variant_t *e, int nesting) {
    enum_variant_t *n;
    for (enum_variant_t *n = e; n != NULL; n = n->next) {
        tb(nesting);
        printf("%2d %20s | %s\n", n->val, n->name, (n->description != NULL) ? n->description : "");
    }
}

void print_data(msg_t *d, int nesting);

void print_field(field_t *f, int nesting) {

    tb(nesting);
    printf ("%s Field \"%s\" class=%s offset=%d size=%d unit=\"%s\" | %s\n",
            f->flags & IBR_FIELD_FLAG_IN_BITFIELD ? "Bit" : "",
            f->name,
            class_caption(f->cls), f->offset, f->size,
            (f->unit[0]) ? f->unit : "-",
            (f->description != NULL) ? f->description : "");

    nesting++;

    switch (f->cls) {
        case fc_scalar:
            tb(nesting);
            printf ("Scalar %s %s %.3f\n", scalar_type_caption(f->nested.scalar_type),
                                                    f->flags & IBR_FIELD_FLAG_HAS_SCALE_CONV ? "scaled" : "",
                                                    f->flags & IBR_FIELD_FLAG_HAS_SCALE_CONV ? f->scale_factor : 1.0);
            break;

        case fc_bitfield:
            tb(nesting);
            printf ("Bit field size=%d\n", f->size);
            print_data(f->nested.bitfield_list, nesting+1);
            break;

        case fc_enum:
            tb(nesting);
            printf ("Enum size=%d\n", f->size);
            print_enum(f->nested.enum_variants_list, nesting+1);
            break;

        case fc_flag:
            //printf ("Flag\n");
            break;

        case fc_array:
            printf ("TODO Array size=%d\n", f->size);
            break;

        case fc_sub_pkt:
            printf ("TODO SubPkt size=%d\n", f->size);
            break;

        default:
            printf ("TODO %d\n", f->cls);
            break;
    }
}

void print_data(msg_t *d, int nesting) {
    tb(nesting);
    printf ("Frame data \"%s\", size=%d : %s\n", d->name, d->size, (d->description != NULL) ? d->description : "");
    for (field_t *n = d->fields_list_head; n != NULL; n = n->next) {
        print_field(n, nesting + 1);
    }
}


int test_payload_config() {

    msg_t d={.name = "timeutc", .description = "UTC time solution"};
    field_t *f, *bf;
    int offset = 0;
    int bits_offset;

    ibr_add_scalar(&d, "iTOW", ft_uint32, &offset, &f);
        ibr_field_annotate(f, "ms", "GPS time of week of the navigation epoch. See the description of iTOW for details.");
    ibr_add_scalar(&d, "tAcc", ft_uint32, &offset, &f);
        ibr_field_annotate(f, "ns", "Time accuracy estimate (UTC)");
    ibr_add_scalar(&d, "nano", ft_int32, &offset, &f);
        ibr_field_annotate(f, "ns", "Fraction of second, range -1e9 .. 1e9 (UTC)");
    ibr_add_scalar(&d, "year", ft_uint16, &offset, &f);
        ibr_field_annotate(f, "y", "Year, range 1999..2099 (UTC)");
    ibr_add_scalar(&d, "month", ft_uint8, &offset, &f);
        ibr_field_annotate(f, "month", "Month, range 1..12 (UTC)");
    ibr_add_scalar(&d, "day", ft_uint8, &offset, &f);
        ibr_field_annotate(f, "f", "Day of month, range 1..31 (UTC)");
    ibr_add_scalar(&d, "hour", ft_uint8, &offset, &f);
        ibr_field_annotate(f, "h", "Hour of day, range 0..23 (UTC)");
    ibr_add_scalar(&d, "min", ft_uint8, &offset, &f);
        ibr_field_annotate(f, "min", "Minute of hour, range 0..59 (UTC)");
    ibr_add_scalar(&d, "sec", ft_uint8, &offset, &f);
        ibr_field_annotate(f, "s", "TSeconds of minute, range 0..60 (UTC)");
    ibr_add_bitfield(&d, "valid", 1, &offset, &bf);
        ibr_field_annotate(f, NULL, "Validity Flags (see graphic below)");
        bits_offset = 0;
        ibr_bitfield_add_element_flag(bf, "validTOW", &bits_offset, &f);
            ibr_field_annotate(f, NULL, "1 = Valid Time of Week");
        ibr_bitfield_add_element_flag(bf, "validWKN", &bits_offset, &f);
            ibr_field_annotate(f, NULL, "1 = Valid Week Number");
        ibr_bitfield_add_element_flag(bf, "validUTC", &bits_offset, &f);
            ibr_field_annotate(f, NULL, "1 = Valid UTC Time");
        bits_offset++;
        ibr_bitfield_add_element_enum(bf, "utcStandard", 4, &bits_offset, &f);
            ibr_field_annotate(f, NULL, "1 = Valid UTC Time");
            ibr_enum_add_variant(f, "na", 0, " Information not available");
            ibr_enum_add_variant(f, "CRL", 1, "Communications Research Labratory (CRL), Tokyo, Japan");
            ibr_enum_add_variant(f, "NIST", 2, "National Institute of Standards and Technology (NIST)");
            ibr_enum_add_variant(f, "USNO", 3, "U.S. Naval Observatory (USNO)");
            ibr_enum_add_variant(f, "BIPM", 4, "International Bureau of Weights and Measures (BIPM)");
            ibr_enum_add_variant(f, "EU", 5, "European laboratories");
            ibr_enum_add_variant(f, "RU", 6, "Former Soviet Union (SU)");
            ibr_enum_add_variant(f, "NTSC", 7, "National Time Service Center (NTSC), China");
            ibr_enum_add_variant(f, "NPLI", 8, "National Physics Laboratory India (NPLI)");
            ibr_enum_add_variant(f, "unknown", 15, "Unknown");

    d.size = offset;

    print_data(&d, 0);

    return 0;
}
