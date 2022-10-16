#include <stdlib.h>
#include "eswb/api.h"
#include "eswb/bridge.h"

#include "function.h"

typedef struct flow_interface {
    const function_inside_flow_t *functions_batch;       /* flow specific, flow invocation independent */
    void **function_handles_batch;                       /* flow invocation specific */

    func_conn_spec_t                *outputs_links;
    const connection_spec_t         **inputs_spec;

    eswb_topic_descr_t root_td;
    eswb_bridge_t **out_bridges;
    eswb_bridge_t **in_bridges;

    int out_bridges_num;
    int in_bridges_num;

    const char *flow_name;
} flow_interface_t;


void* flow_alloc(size_t s);


int flow_batch_size(const function_inside_flow_t *batch_array) {
    int i;
    for (i = 0; batch_array[i].h != NULL; i++);
    return i;
}

static const function_calls_t flow_calls_template;

void flow_get_handler(function_flow_t *flow, function_handler_t *fh) {
    fh->spec = &flow->spec;
    fh->extension_handler = &flow->cfg; // must be aligned with flow_init type
    fh->calls = &flow_calls_template;
}

static int check_connectivity(const char *flow_name, const char *inv_name, const char *spec_name, const char *entity,
                                     const func_conn_spec_t *func_conn, const connection_spec_t **conn_spec) {
    int err_cnt = 0;
    // check func_conn are in conn_spec
    if ((func_conn != NULL) && (conn_spec != NULL)) {
        for (int i = 0; func_conn[i].alias != NULL; i++) {
            if (fspec_conn_find(conn_spec, func_conn[i].alias) == NULL) {
                dbg_msg("Flow \"%s\" | Invocation \"%s\" of \"%s\" references non declared %s: \"%s\"",
                        flow_name, inv_name, spec_name, entity, func_conn[i].alias);
                err_cnt++;
            }
        }
    }

    return err_cnt;
}

#define flow_init_dbg_msg(__frv, __callname, __func_invk, __flow_name)    dbg_msg_ec(__frv, "%s failed for invocation \"%s\" of \"%s\" in flow \"%s\"", \
                                __callname,                                                                                             \
                                (__func_invk)->name,                                                                                    \
                                (__func_invk)->h->spec->name,                                                                           \
                                __flow_name)

fspec_rv_t flow_init(void *iface, const function_spec_t *spec, const char *inv_name, eswb_topic_descr_t mounting_td, const void *extension_handler) {
    flow_interface_t *flow_dh = (flow_interface_t *)iface;
    int err_cnt = 0;
    fspec_rv_t frv;

    const function_flow_cfg_t *fl_cfg = (const function_flow_cfg_t *) extension_handler;
    flow_dh->functions_batch = fl_cfg->functions_batch;
    flow_dh->outputs_links = fl_cfg->outputs_links;
    flow_dh->inputs_spec = spec->inputs;
    flow_dh->flow_name = spec->name;

    int batch_size = flow_batch_size(flow_dh->functions_batch );

    if (batch_size > 0) {
        flow_dh->function_handles_batch = flow_alloc((batch_size + 1) * sizeof (*flow_dh->function_handles_batch));
        if (flow_dh->function_handles_batch == NULL) {
            return fspec_rv_no_memory;
        }
    } else {
        return fspec_rv_empty;
    }

    for (int i = 0; i < batch_size; i++) {
        err_cnt += check_connectivity(flow_dh->flow_name,
                                      flow_dh->functions_batch[i].name,
                                      flow_dh->functions_batch[i].h->spec->name,
                                      "input",
                                      flow_dh->functions_batch[i].connect_spec,
                                      flow_dh->functions_batch[i].h->spec->inputs);

        err_cnt += check_connectivity(flow_dh->flow_name,
                                      flow_dh->functions_batch[i].name,
                                      flow_dh->functions_batch[i].h->spec->name,
                                      "param",
                                      flow_dh->functions_batch[i].initial_params,
                                      flow_dh->functions_batch[i].h->spec->params);
    }

    eswb_rv_t rv;
    char flow_root_path[ESWB_TOPIC_NAME_MAX_LEN + 1];

    if (mounting_td == 0) {
        // TODO count topics num
        char bus_name[ESWB_BUS_NAME_MAX_LEN + 1];
        #define FLOW_PREFIX "_flow_"
        strcpy(bus_name, FLOW_PREFIX);
        strncat(bus_name, inv_name, ESWB_BUS_NAME_MAX_LEN - strlen(FLOW_PREFIX));

        rv = eswb_create(bus_name, eswb_non_synced, 256);
        if (rv != eswb_e_ok) {
            dbg_msg("eswb_create error: %s", eswb_strerror(rv));
            return fspec_rv_initerr;
        }

        rv = eswb_path_compose(eswb_non_synced, bus_name, NULL, flow_root_path);
        if (rv != eswb_e_ok) {
            dbg_msg("eswb_path_compose error: %s", eswb_strerror(rv));
            return fspec_rv_invarg;
        }
    } else {
        rv = eswb_get_topic_path(mounting_td, flow_root_path);
        if (rv != eswb_e_ok) {
            dbg_msg("eswb_get_topic_path error: %s", eswb_strerror(rv));
            return fspec_rv_invarg;
        }

        eswb_mkdir(flow_root_path, inv_name);
        // reusing same variable:
        strncat(flow_root_path, inv_name, ESWB_BUS_NAME_MAX_LEN - strlen(flow_root_path));
    }

    rv = eswb_connect(flow_root_path, &mounting_td);
    if (rv != eswb_e_ok) {
        dbg_msg("eswb_connect error: %s", eswb_strerror(rv));
        return fspec_rv_no_topic;
    }

    flow_dh->root_td = mounting_td;

    for (int i = 0; i < batch_size; i++) {
        flow_dh->function_handles_batch[i] = NULL;
        frv = function_init(flow_dh->functions_batch[i].h, flow_dh->functions_batch[i].name, mounting_td,
                            &flow_dh->function_handles_batch[i]);
        if ((frv != fspec_rv_ok) && (frv != fspec_rv_not_supported)) { // it is ok not to have this callback
            flow_init_dbg_msg(frv, __func__, &flow_dh->functions_batch[i], flow_dh->flow_name);
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

static fspec_rv_t bridge_the_flow(const func_conn_spec_t *src, const func_conn_spec_t *dst,
                                  eswb_topic_descr_t src_mtd, eswb_topic_descr_t dst_mtd, eswb_bridge_t ***br_rv, int *br_num) {
    int errs = 0;
    // TODO signle bridge or signal by signal
    int links_num;
    for (links_num = 0; src[links_num].alias != NULL; links_num++);

    eswb_bridge_t **br = flow_alloc(links_num * sizeof(eswb_bridge_t *));
    //char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

     for (int i = 0; i < links_num; i++) {
        eswb_rv_t erv = eswb_bridge_create(src[i].alias, 1, &br[i]);
        if (erv !=  eswb_e_ok) {
            dbg_msg("eswb_bridge_create failed: %s", eswb_strerror(erv));
            continue;
        }

        const char *dst_path = fspec_find_path2connect(dst, src[i].alias);
        if (dst_path == NULL) {
            dbg_msg("Terminal %s have no specified connection path", src[i].alias);
            continue;
        }

        erv = eswb_bridge_add_topic(br[i], src_mtd, src[i].value, NULL);
        if (erv != eswb_e_ok) {
            dbg_msg("eswb_bridge_add_topic to \"%s\" failed: %s", src[i].value, eswb_strerror(erv));
            errs++;
        }

        erv = eswb_bridge_connect_scalar(br[i], dst_mtd, dst_path);
        if (erv != eswb_e_ok) {
            dbg_msg("eswb_bridge_connect_scalar failed: %s", eswb_strerror(erv));
            errs++;
        }
    }

    if (errs == 0) {
        *br_rv = br;
        *br_num = links_num;
    }

    return errs > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

fspec_rv_t flow_init_outputs(void *dhandle, const func_conn_spec_t *conn_spec,
                             eswb_topic_descr_t mounting_point, const char *func_name) {

    flow_interface_t *flow_dh = (flow_interface_t *) dhandle;
    int i = 0;
    int errs = 0;

    fspec_rv_t frv;

    if (mounting_point == 0) {
        mounting_point = flow_dh->root_td;
    }

    // init control seq outputs
    while (flow_dh->functions_batch[i].h != NULL) {
        frv = function_init_outputs(flow_dh->functions_batch[i].h,
                                    flow_dh->function_handles_batch[i],
                                    NULL,
                                    mounting_point, flow_dh->functions_batch[i].name);
        if ((frv != fspec_rv_ok) && (frv != fspec_rv_not_supported)) { // it is ok not to have no outputs
            flow_init_dbg_msg(frv, __func__, &flow_dh->functions_batch[i], flow_dh->flow_name);
            errs++;
        }
        i++;
    }

    // TODO if conn_spec is NULL, then mount all outputs to local dir

    frv = bridge_the_flow(flow_dh->outputs_links, conn_spec,
                          mounting_point, 0,
                          &flow_dh->out_bridges, &flow_dh->out_bridges_num);

    if (frv != fspec_rv_ok) {
        errs++;
    }

    return errs > 0 ? fspec_rv_initerr : fspec_rv_ok;
}


fspec_rv_t flow_init_inputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point) {
    flow_interface_t *flow_dh = (flow_interface_t *) dhandle;

    int i;
    int errs = 0;
    fspec_rv_t rv = fspec_rv_ok;
    fspec_rv_t frv;

    if (mounting_point == 0) {
        mounting_point = flow_dh->root_td;
    }

    // FIXME max inputs
#   define MAX_INPUTS 16
    func_conn_spec_t inputs_bridging_spec[MAX_INPUTS + 1];
    for (i = 0; flow_dh->inputs_spec[i] != NULL; i++) {
        if (i >= MAX_INPUTS) {
            dbg_msg("MAX_INPUTS exceeded!");
            errs++;
            break;
        }
        inputs_bridging_spec[i].value = inputs_bridging_spec[i].alias = flow_dh->inputs_spec[i]->name;
    }

    inputs_bridging_spec[i].alias = NULL;
    if (i > 0) {
        frv = bridge_the_flow(conn_spec, inputs_bridging_spec, 0, mounting_point, &flow_dh->in_bridges, &flow_dh->in_bridges_num);
        if (frv != fspec_rv_ok) {
            dbg_msg_ec(frv, "bridge_the_flow failed");
            errs++;
        }
    }

    i = 0;
    while (flow_dh->functions_batch[i].h != NULL) {
        frv = function_init_inputs(flow_dh->functions_batch[i].h, flow_dh->function_handles_batch[i],
                                   flow_dh->functions_batch[i].connect_spec,
                                   mounting_point);
        if ((frv != fspec_rv_ok) && (frv != fspec_rv_not_supported)) { // it is ok not to have inputs
            flow_init_dbg_msg(frv, __func__, &flow_dh->functions_batch[i], flow_dh->flow_name);
            errs++;
        }
        i++;
    }

    return errs > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

fspec_rv_t flow_set_invocation_params(flow_interface_t *flow_dh, const char *inv_name, const func_param_t *param);
fspec_rv_t flow_find_invocation(flow_interface_t *flow_dh, const char *inv_name, const function_inside_flow_t **inv, void **dhandle);

fspec_rv_t flow_set_params(void *dhandle, const func_param_t *params, int initial_call) {
    flow_interface_t *flow_dh = (flow_interface_t *) dhandle;

    int i = 0;
    int errs = 0;
    fspec_rv_t frv;


    if (initial_call) {
        while (flow_dh->functions_batch[i].h != NULL) {
            frv = function_set_param(flow_dh->functions_batch[i].h, flow_dh->function_handles_batch[i],
                                     flow_dh->functions_batch[i].initial_params, initial_call);

            if ((frv != fspec_rv_ok) && (frv != fspec_rv_not_supported)) { // it is ok not to have inputs
                flow_init_dbg_msg(frv, __func__, &flow_dh->functions_batch[i], flow_dh->flow_name);
                errs++;
            }
            i++;
        }
    } else {
        // func name supposed to be the first parameter in a array
        return flow_set_invocation_params(flow_dh, params[0].value, &params[1]);
    }

    return errs > 0 ? fspec_rv_inval_param : fspec_rv_ok;
}


void flow_exec(void *dhandle) {
    flow_interface_t *flow_dh = (flow_interface_t *) dhandle;
    int i;

    for (i = 0; i < flow_dh->in_bridges_num; i++) {
        eswb_bridge_update(flow_dh->in_bridges[i]);
    }

    i = 0;
    while (flow_dh->functions_batch[i].h != NULL) {
        flow_dh->functions_batch[i].h->calls->exec(flow_dh->function_handles_batch[i]);
        i++;
    }

    for (i = 0; i < flow_dh->out_bridges_num; i++) {
        eswb_bridge_update(flow_dh->out_bridges[i]);
    }
}



fspec_rv_t flow_find_invocation(flow_interface_t *flow_dh, const char *inv_name,
                                const function_inside_flow_t **inv,
                                void **dhandle) {

    if (inv_name == NULL) {
        return fspec_rv_inval_param;
    }

    for (int i = 0; flow_dh->functions_batch[i].h != NULL; i++) {
        if (strcmp(flow_dh->functions_batch[i].name, inv_name) == 0){
            *inv = &flow_dh->functions_batch[i];
            *dhandle = flow_dh->function_handles_batch[i];
            return fspec_rv_ok;
        }
    }

    return fspec_rv_not_exists;
}

fspec_rv_t flow_set_invocation_params(flow_interface_t *flow_dh, const char *inv_name, const func_param_t *param) {

    const function_inside_flow_t *inv;
    void *dhandle;

    fspec_rv_t rv = flow_find_invocation(flow_dh, inv_name, &inv, &dhandle);
    if (rv != fspec_rv_ok) {
        return rv;
    }

    rv = function_set_param(inv->h, dhandle, param, 0);

    return rv;
}

static const function_calls_t flow_calls_template = {
    .interface_handle_size = sizeof(flow_interface_t),
    .init = flow_init,
    .init_outputs = flow_init_outputs,
    .init_inputs = flow_init_inputs,
    .set_params = flow_set_params,
    .exec = flow_exec
};
