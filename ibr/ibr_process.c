#include <stdint.h>
#include <stdlib.h>
#include <eswb/api.h>




#include "ibr.h"
#include "ibr_process.h"
#include "ibr_convert.h"

void *ibr_alloc(size_t s);

topic_data_type_t ibr_eswb_field_type(field_type_t *ft) {
    field_scalar_type_t st = ft->st;

    switch (ft->cls) {
        case fc_bitfield:
            st = ibr_get_equivalent_type_for_bitfield_size(ft->size);
            // fall through

        case fc_scalar:
            switch (st) {
                default:
                case ft_invalid:    return tt_none;
                case ft_float:      return tt_float;
                case ft_double:     return tt_double;
                case ft_uint8:      return tt_uint8;
                case ft_int8:       return tt_int8;
                case ft_uint16:     return tt_uint16;
                case ft_int16:      return tt_int16;
                case ft_uint32:     return tt_uint32;
                case ft_int32:      return tt_int32;
                case ft_uint64:     return tt_uint64;
                case ft_int64:      return tt_int64;
            }

        case fc_array:              return tt_string;
        default:                    return tt_none;
    }
}

const irb_media_driver_t irb_media_driver_eswb;
const irb_media_driver_t irb_media_driver_eswb_bridge;
const irb_media_driver_t irb_media_driver_eswb_vector;
const irb_media_driver_t irb_media_driver_udp;

const irb_media_driver_t irb_media_driver_file;
const irb_media_driver_t irb_media_driver_sdtl;
const irb_media_driver_t irb_media_driver_serial;



const irb_media_driver_t *ibr_get_driver (irb_media_driver_type_t mdt) {
    switch (mdt) {
        case mdt_function:              return &irb_media_driver_eswb;
        case mdt_function_bridge:       return &irb_media_driver_eswb_bridge;
        case mdt_function_vector:       return &irb_media_driver_eswb_vector;
#       ifndef CATOM_NO_SOCKET
        case mdt_udp:                   return &irb_media_driver_udp;
#       endif
        case mdt_file:                  return &irb_media_driver_file;
        case mdt_serial:                return &irb_media_driver_serial;
        case mdt_sdtl:                  return &irb_media_driver_sdtl;
        default:                        return NULL;
    }
}


ibr_rv_t ibr_decode_addr(const char *addr, irb_media_driver_type_t *mdt, const char **addr_rv) {
    char drv_alias[21] = "";
#define PATH_SEPARATOR "://"
    if (sscanf(addr, "%20[a-z_]" PATH_SEPARATOR, drv_alias) < 1) {
        return ibr_invarg;
    }

    if (strcmp(drv_alias, "func") == 0) {
        *mdt = mdt_function;
    } else if (strcmp(drv_alias, "func_br") == 0) {
        *mdt = mdt_function_bridge;
    } else if (strcmp(drv_alias, "func_vec") == 0) {
        *mdt = mdt_function_vector;
    } else if (strcmp(drv_alias, "udp") == 0) {
        *mdt = mdt_udp;
    } else if (strcmp(drv_alias, "file") == 0) {
        *mdt = mdt_file;
    } else if (strcmp(drv_alias, "serial") == 0) {
        *mdt = mdt_serial;
    } else if (strcmp(drv_alias, "sdtl") == 0) {
        *mdt = mdt_sdtl;
    } else {
        return ibr_invarg;
    }

    *addr_rv = addr + strlen(drv_alias) + strlen(PATH_SEPARATOR);

    return ibr_ok;
}

void ibr_set_pthread_name(const char *tn);

/*
static inline ibr_rv_t ibr_process (irb_process_setup_t *setup) {
    int msg_size = setup->dst_msg->size;
    const irb_media_driver_t *dev_src = setup->src.drv;
    const irb_media_driver_t *dev_dst = setup->dst.drv;

    void *src_buf = ibr_alloc(msg_size);
    void *dst_buf = ibr_alloc(msg_size);

    int srcd = setup->src.descr;
    int dstd = setup->dst.descr;

    ibr_rv_t rv;

    conv_instr_queue_t *conv_instrs = &setup->conv_instrs;

    ibr_set_pthread_name(setup->name);

    while(1) {
        int br = msg_size;
        rv = dev_src->recv(srcd, src_buf, &br);
        if (rv == ibr_ok) {

            // TODO convert format, apply rules

            conv_exec(conv_instrs, src_buf, msg_size, dst_buf, msg_size);

            int bw = msg_size;
            rv = dev_dst->send(dstd, dst_buf, &bw);
            if (rv != ibr_ok) {
                // TODO ?
                break;
            }
        }
    }

    return rv;
}
*/

msg_record_t *resolve_message_record(irb_process_setup_t *s, uint32_t id) {

    for (int i = 0; i < s->msgs_num; i++) {
        if (s->msgs_setup[i].id == id) {
            return &s->msgs_setup[i];
        }
    }

    return NULL;
}

uint32_t resolve_scalar_int(field_t *f, void *frame_start) {
    uint32_t rv = 0;

//    void *var_start = frame_start + f->offset;
//    // FIXME me: unaligned access
//    switch (f->nested.scalar_type) {
//        case ft_uint8:
//            rv = *((uint8_t *)frame_start);
//            break;
//
//        case ft_uint16:
//            rv = *((uint16_t *)frame_start);
//            break;
//
//        case ft_uint32:
//            rv = *((uint32_t *)frame_start);
//            break;
//
//        default:
//            rv = UINT32_MAX;
//    }

    memcpy(&rv, frame_start + f->offset, f->size);

    return rv;
}

static inline ibr_rv_t ibr_process_frame (irb_process_setup_t *setup) {
    int msg_size = 512;

    const irb_media_driver_t *dev_src = setup->src.drv;
    const irb_media_driver_t *dev_dst = setup->dst.drv;

    void *src_buf = ibr_alloc(msg_size);
    void *dst_buf = ibr_alloc(msg_size);

    void *src_buf_payload = src_buf + setup->frame->payload_offset;

    int srcd = setup->src.descr;

    ibr_rv_t rv;

    ibr_set_pthread_name(setup->name);



    while(1) {
        int br = msg_size;
        rv = dev_src->recv(srcd, src_buf, &br);
        if (rv == ibr_ok) {

            // TODO frame fields are optional
            uint32_t resolved_id = resolve_scalar_int(setup->frame->resolve_id, src_buf);
            uint32_t resolved_len = resolve_scalar_int(setup->frame->resolve_len, src_buf);

            msg_record_t *m = resolve_message_record(setup, resolved_id);

            if (m == NULL) {
                continue;
            }
            if (m->src_msg->size != resolved_len) {
                printf("%s | INVALID LEN | frame id=0x%02X expect len=%d got=%d (br=%d)\n", __func__, resolved_id, m->src_msg->size, resolved_len, br);
                continue;
            }

            printf("%s | resolved id=0x%02X len=%d\n", __func__, resolved_id, resolved_len);

            int bw = conv_exec(&m->conv_queue, src_buf_payload, m->src_msg->size,
                               dst_buf, m->src_msg->size);

            rv = dev_dst->send(m->descr, dst_buf, &bw);
            if (rv != ibr_ok) {
                // TODO ?
                break;
            }

        }
    }

    return rv;
}



void *ibr_process_thread (void *setup) {
    ibr_process_frame((irb_process_setup_t *) setup);
    return NULL;
}

ibr_rv_t ibr_process_start (irb_process_setup_t *setup) {
    int rv = pthread_create(&setup->tid, NULL, ibr_process_thread, setup);

    return rv ? ibr_processerr : ibr_ok;
}
