#include <eswb/api.h>
#include <eswb/bridge.h>

#include "ibr.h"
#include "ibr_process.h"


#define MAX_BRIDGES 5
static int bridges_num = 0;
static eswb_bridge_t *bridges[MAX_BRIDGES];

ibr_rv_t drv_eswb_disconnect(int td);

field_type_t ibr_field_type_from_eswb(topic_data_type_t d) {
    field_type_t rv;
    switch (d) {
        default:
        case tt_none:       rv.cls = fc_scalar; rv.st = ft_invalid; break;
        case tt_float:      rv.cls = fc_scalar; rv.st = ft_float; break;
        case tt_double:     rv.cls = fc_scalar; rv.st = ft_double; break;
        case tt_uint8:      rv.cls = fc_scalar; rv.st = ft_uint8; break;
        case tt_int8:       rv.cls = fc_scalar; rv.st = ft_int8; break;
        case tt_uint16:     rv.cls = fc_scalar; rv.st = ft_uint16; break;
        case tt_int16:      rv.cls = fc_scalar; rv.st = ft_int16; break;
        case tt_uint32:     rv.cls = fc_scalar; rv.st = ft_uint32; break;
        case tt_int32:      rv.cls = fc_scalar; rv.st = ft_int32; break;
        case tt_uint64:     rv.cls = fc_scalar; rv.st = ft_uint64; break;
        case tt_int64:      rv.cls = fc_scalar; rv.st = ft_int64; break;

        case tt_string:     rv.cls = fc_array; rv.st = ft_uint8; break;
    }

    return rv;
}

static eswb_rv_t create_bridge(const char *path, ibr_msg_t *dst_msg, ibr_msg_t *src_msg, eswb_topic_descr_t *td) {


    if (!bridges_num) {
        memset(bridges, 0, sizeof(bridges));
    }

    char tpath [ESWB_TOPIC_MAX_PATH_LEN + 1];
    strncpy(tpath, path, ESWB_TOPIC_MAX_PATH_LEN - 1);
    strcat(tpath, "/");
    size_t path_len = strlen(tpath);

    if (bridges_num >= MAX_BRIDGES) {
        return eswb_e_mem_data_na;
    }

    if (ibr_msg_fields_num(src_msg) != 0) {
        return eswb_e_invargs;
    }

    eswb_bridge_t *br = NULL;
    eswb_rv_t rv = eswb_bridge_create(src_msg->name,
                                      ibr_msg_fields_num(dst_msg),
                                      &br);
    if (rv != eswb_e_ok) {
        return rv;
    }

    int offset = 0;

    for (field_t *f = dst_msg->fields_list_head; f != NULL; f = f->next) {
        tpath[path_len] = 0;
        strncat(tpath, f->name, ESWB_TOPIC_MAX_PATH_LEN - path_len);
        ibr_rv_t irv;

        rv = eswb_bridge_add_topic(br, 0, tpath, f->name);
        if (rv == eswb_e_ok) {
            field_type_t ft = ibr_field_type_from_eswb(br->topics[br->tds_num-1].type); // nasty: last added topic, from prev call
            int field_size = (int) br->topics[br->tds_num-1].size;
            switch (ft.cls) {
                case fc_scalar:
                    irv = ibr_add_scalar(src_msg, f->name,
                                         ft.st,
                                         &offset, NULL);
                    break;

                case fc_array:
                    irv = ibr_add_array(src_msg, f->name,
                                        ibr_get_scalar_size(ft.st), field_size,
                                        &offset, NULL);
                    break;

                default:
                    irv = ibr_invarg;
                    break;
            }
        } else if (rv == eswb_e_no_topic) {
            irv = ibr_add_dummy(src_msg, f->name, f->size, &offset, NULL);
        } else {
            return rv;
        }

        if (irv != ibr_ok) {
            return eswb_e_invargs;
        }
    }

    if (br->tds_num == 0) {
        return eswb_e_no_topic;
    }

    *td = bridges_num;
    bridges[bridges_num] = br;
    bridges_num++;

    return eswb_e_ok;
}


ibr_rv_t drv_eswb_bridge_proclaim(const char *path, ibr_msg_t *src_msg, ibr_msg_t *dst_msg, int *td) {
    eswb_rv_t rv;

    rv = create_bridge(path, dst_msg, src_msg, td);
    if (rv != eswb_e_ok) {
        dbg_msg("create_bridge failed: %s", eswb_strerror(rv));
        return ibr_media_err;
    }

    return ibr_ok;
}


ibr_rv_t drv_eswb_bridge_recv(int td, void *d, int *btr) {
    eswb_rv_t rv;
    eswb_bridge_t *br = bridges[td];

    // clocking using last topic
    rv = eswb_get_update(br->topics[br->tds_num-1].td, NULL);
    if (rv == eswb_e_ok) {
        rv = eswb_bridge_read(bridges[td], d);
    }

    *btr = (int)br->buffer2post_size;

    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}


const irb_media_driver_t irb_media_driver_eswb_bridge = {
        .proclaim = drv_eswb_bridge_proclaim,
        .connect = NULL,
        .disconnect = drv_eswb_disconnect,
        .send = NULL,
        .recv = drv_eswb_bridge_recv,
};
