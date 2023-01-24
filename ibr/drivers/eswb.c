#include <eswb/api.h>

#include "ibr.h"
#include "ibr_process.h"




static eswb_rv_t proclaim_msg(msg_t *m, const char *dst_path, eswb_topic_descr_t *td) {

    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];
    char struct_topic_name[ESWB_TOPIC_NAME_MAX_LEN + 1];

    eswb_rv_t rv = eswb_path_split(dst_path, path, struct_topic_name);

    if (rv != eswb_e_ok) {
        return rv;
    }

    int fields_num = ibr_msg_fields_num(m);
    if (fields_num == 0) {
        dbg_msg("Message has no fields");
        return eswb_e_invargs;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, fields_num + 1);

    char *tn = strlen(struct_topic_name) == 0 ? m->name : struct_topic_name;

    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, tn, tt_struct, m->size);
    if (rt == NULL) {
        return eswb_e_invargs;
    }
    eswb_size_t offset = 0;

    // prepare topic struct
    for (field_t *n = m->fields_list_head; n != NULL; n = n->next) {
//        if (n->cls != fc_scalar) {
//            dbg_msg("Non scalar type is not supported for IBR");
//            return eswb_e_invargs;
//        }
        field_type_t ft;
        ft.cls = n->cls;
        ft.st = n->nested.scalar_type;
        ft.size = n->size;

        topic_data_type_t eswb_data_type = ibr_eswb_field_type(&ft);
        if (eswb_data_type == tt_none) {
            dbg_msg("Non supported type (%d)", n->nested.scalar_type);
            return eswb_e_invargs;
        }

        usr_topic_add_child(cntx, rt,
                            n->name,
                            eswb_data_type,
                            n->offset,
                            n->size, TOPIC_PROCLAIMING_FLAG_MAPPED_TO_PARENT);
    }

    rv = eswb_proclaim_tree_by_path(path, rt, cntx->t_num, td);
    if (rv != eswb_e_ok) {
        dbg_msg("Proclaiming error: %s", eswb_strerror(rv));
    }

    return rv;
}

ibr_rv_t drv_eswb_proclaim(const char *path, msg_t *src_msg, msg_t *dst_msg, int *td) {
    eswb_rv_t rv;

    rv = proclaim_msg(dst_msg, path, td);
    if (rv != eswb_e_ok) {
        dbg_msg("proclaim_msg failed: %s", eswb_strerror(rv));
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_eswb_connect(const char *path, int *td) {
    eswb_rv_t rv;
    rv = eswb_connect(path, td);
    return rv == eswb_e_ok ? ibr_ok : ibr_nomedia;
}

ibr_rv_t drv_eswb_disconnect(int td) {
    eswb_rv_t rv;
    rv = eswb_disconnect (td);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_send(int td, void *d, int *bts) {
    eswb_rv_t rv;
    rv = eswb_update_topic(td, d);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_recv(int td, void *d, int *btr) {
    eswb_rv_t rv;
    rv = eswb_get_update(td, d);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}


const irb_media_driver_t irb_media_driver_eswb = {
        .proclaim = drv_eswb_proclaim,
        .connect = drv_eswb_connect,
        .disconnect = drv_eswb_disconnect,
        .send = drv_eswb_send,
        .recv = drv_eswb_recv,
};
