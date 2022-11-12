#include <stdlib.h>
#include <eswb/api.h>
#include <eswb/errors.h>

#include "ebr.h"



typedef struct ebr_handle {
    ebr_instance_t *instance;
} ebr_handle_t;

fspec_rv_t ebr_init(void *iface, const function_spec_t *spec, const char *inv_name, eswb_topic_descr_t mounting_td, const void *extension_handler) {
    ebr_handle_t *ebr = iface;

    ebr->instance = (ebr_instance_t *) extension_handler;

    return fspec_rv_ok;
}


fspec_rv_t ebr_init_outputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point, const char *func_name) {
    ebr_instance_t *ebr = ((ebr_handle_t *)(dhandle))->instance;
    int err_connect_cnt = 0;
    fspec_rv_t rv = fspec_rv_ok;

    // bridge supposed to be initialized after its sources, otherwise will not work

    eswb_rv_t erv;
    for (unsigned i = 0; ebr->sources_paths[i].alias != NULL; i++) {
        erv = eswb_bridge_add_topic(ebr->bridge, 0, ebr->sources_paths[i].value, ebr->sources_paths[i].alias);
        if (erv != eswb_e_ok) {
            dbg_msg("eswb_bridge_add_topic error for \"%s\": %s", ebr->sources_paths[i].value, eswb_strerror(erv));
            err_connect_cnt++;
        }
    }

    if (err_connect_cnt == 0) {
        erv = eswb_bridge_connect_vector(ebr->bridge, ebr->dst_path);
        if (erv != eswb_e_ok) {
            rv = fspec_rv_publish_err;
        }
    } else {
        rv = fspec_rv_no_topic;
    }

    return rv;
}


fspec_rv_t ebr_init_inputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point) {
    ebr_instance_t *ebr = ((ebr_handle_t *)(dhandle))->instance;
    eswb_rv_t erv;

    // already inited

    return fspec_rv_ok;
}


void ebr_exec(void *dhandle) {
    ebr_instance_t *ebr = ((ebr_handle_t *)(dhandle))->instance;

    eswb_bridge_update(ebr->bridge);
}

static const function_calls_t ebr_calls = {
        .interface_handle_size = sizeof(ebr_handle_t),
        .init = ebr_init,
        .init_outputs = ebr_init_outputs,
        .init_inputs = ebr_init_inputs,
        .set_params = NULL,
        .pre_exec_init = NULL,
        .exec = ebr_exec
};


void ebr_get_handler(ebr_instance_t *ebr, function_handler_t *fh) {
    fh->spec = NULL;
    fh->extension_handler = ebr;
    fh->calls = &ebr_calls;
}
