#include <eswb/api.h>
#include "function.h"
#include "ibr_process.h"

msg_t *ibr_find_msg(const ibr_cfg_t *ibr, const char *msg_name) {
    for(int i = 0; ibr->protocol.msgs[i] != NULL; i++) {
        if (strcmp(ibr->protocol.msgs[i]->name, msg_name) == 0) {
            return ibr->protocol.msgs[i];
        }
    }

    return NULL;
}

static int mtd_is_function(irb_media_driver_type_t mdt) {
    return mdt == mdt_function || mdt == mdt_function_bridge;
}

ibr_rv_t ibr_process_setup(ibr_cfg_t *ibr_cfg, const function_spec_t *spec,
                           process_cfg_t *proc_cfg, irb_process_setup_t *setup) {

    ibr_rv_t rv;

    setup->name = proc_cfg->name;

    setup->dst_msg = ibr_find_msg(ibr_cfg, proc_cfg->msg);
    if (setup->dst_msg == NULL) {
        dbg_msg("No message \"%s\" for IBR \"%s\"", proc_cfg->msg, ibr_cfg->spec.name);
        return ibr_noent;
    }

    rv = ibr_decode_addr(proc_cfg->src, &setup->src.mdt, &setup->src.access_addr);
    if (rv !=ibr_ok) {
        dbg_msg("Invalid source (%s) specification for IBR \"%s\"", proc_cfg->src, ibr_cfg->spec.name);
        return rv;
    }

    rv = ibr_decode_addr(proc_cfg->dst, &setup->dst.mdt, &setup->dst.access_addr);
    if (rv !=ibr_ok) {
        dbg_msg("Invalid dest (%s) specification for IBR \"%s\"", proc_cfg->src, ibr_cfg->spec.name);
        return rv;
    }

    setup->src.drv = ibr_get_driver (setup->src.mdt);
    setup->dst.drv = ibr_get_driver (setup->dst.mdt);

    if (setup->src.drv == NULL || setup->dst.drv == NULL) {
        dbg_msg("Drivers setup error for IBR \"%s\"", ibr_cfg->spec.name);
        return ibr_nomedia;
    }

    if (mtd_is_function(setup->dst.mdt)) {
        const connection_spec_t *cs = fspec_conn_find(spec->outputs, setup->dst.access_addr);
        if (cs == NULL) {
            dbg_msg("There is no output \"%s\" referenced by dst of \"%s\"",
                    setup->dst.access_addr,
                    proc_cfg->name);
            return ibr_invarg;
        }
    }

    if (mtd_is_function(setup->src.mdt)) {
        const connection_spec_t *cs = fspec_conn_find(spec->inputs, setup->src.access_addr);
        if (cs == NULL) {
            dbg_msg("There is no input \"%s\" referenced by src of \"%s\"",
                    setup->src.access_addr,
                    proc_cfg->name);
            return ibr_invarg;
        }
    }

    // TODO compile message conversion instructions batch

    return ibr_ok;
}

void *ibr_alloc(size_t s);

fspec_rv_t ibr_init(void *dhandle, const function_spec_t *spec, const char *inv_name, eswb_topic_descr_t mounting_td, const void *extension_handler) {
    int err_cnt = 0;
    ibr_cfg_t *ibr_cfg = (ibr_cfg_t *) extension_handler;
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    ibr_rv_t rv;

    ibr_setup->process_setups = ibr_alloc(ibr_cfg->processes_num * sizeof(irb_process_setup_t));
    ibr_setup->processes_num = ibr_cfg->processes_num;

    for (int i = 0; i < ibr_cfg->processes_num; i++) {
        rv = ibr_process_setup(ibr_cfg, spec, &ibr_cfg->processes[i], &ibr_setup->process_setups[i]);
        if (rv != ibr_ok) {
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

fspec_rv_t ibr_init_outputs(void *dhandle, const func_conn_spec_t *conn_spec,
                             eswb_topic_descr_t mounting_point, const char *func_name) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    int err_cnt = 0;

    for (int i = 0; i < ibr_setup->processes_num; i++) {
        ibr_rv_t rv;
        if (ibr_setup->process_setups[i].dst.drv->proclaim != NULL) {
            const char *eswb_out_path = fspec_find_path2connect(conn_spec, ibr_setup->process_setups[i].dst.access_addr);
            if (eswb_out_path == NULL) {
                dbg_msg("Output \"%s\" have no specified connection", ibr_setup->process_setups[i].dst.access_addr);
                err_cnt++;
                continue;
            }
            rv = ibr_setup->process_setups[i].dst.drv->proclaim(eswb_out_path, NULL, ibr_setup->process_setups[i].dst_msg,
                                                                         &ibr_setup->process_setups[i].dst.descr);
        } else {
            rv = ibr_setup->process_setups[i].dst.drv->connect(ibr_setup->process_setups[i].dst.access_addr,
                                                                         &ibr_setup->process_setups[i].dst.descr);
        }
        if (rv != ibr_ok) {
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}


fspec_rv_t ibr_init_inputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    int err_cnt = 0;

    for (int i = 0; i < ibr_setup->processes_num; i++) {
        ibr_rv_t rv;

        const char *in_path;

        if (ibr_setup->process_setups[i].src.mdt == mdt_function ||
                ibr_setup->process_setups[i].src.mdt == mdt_function_bridge) {
            in_path = fspec_find_path2connect(conn_spec, ibr_setup->process_setups[i].src.access_addr);
            if (in_path == NULL) {
                dbg_msg("Input \"%s\" have no specified connection", ibr_setup->process_setups[i].src.access_addr);
                err_cnt++;
                continue;
            }
        } else {
            in_path = ibr_setup->process_setups[i].src.access_addr;
        }

        if (ibr_setup->process_setups[i].src.drv->proclaim != NULL) {
            msg_t *msg = ibr_alloc(sizeof(msg_t));
            if (msg == NULL) {
                return fspec_rv_no_memory;
            }
            msg->name = conn_spec->alias;

            ibr_setup->process_setups[i].src_msg = msg;

            rv = ibr_setup->process_setups[i].src.drv->proclaim(in_path,
                                                                ibr_setup->process_setups[i].src_msg,
                                                                ibr_setup->process_setups[i].dst_msg,
                                                                &ibr_setup->process_setups[i].src.descr);
        } else {
            rv = ibr_setup->process_setups[i].src.drv->connect(in_path,
                                                               &ibr_setup->process_setups[i].src.descr);
        }
        if (rv != ibr_ok) {
            dbg_msg("Input \"%s\" for IBR process \"%s\" failed (%d)",
                    ibr_setup->process_setups[i].src.access_addr,
                    ibr_setup->process_setups[i].name,
                    rv);
            err_cnt++;
        }

        // TODO compare sizes
    }

    // TODO init conversion? or right after inputs init ?

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}


fspec_rv_t ibr_set_params(void *dhandle, const func_param_t *params, int initial_call) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;

    return fspec_rv_ok;
}


void ibr_exec(void *dhandle) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    int err_cnt = 0;
    ibr_rv_t rv;

    for (int i = 0; i < ibr_setup->processes_num; i++) {
        rv = ibr_process_start(&ibr_setup->process_setups[i]);
        if (rv != ibr_ok) {
            dbg_msg("ibr_process_start failed");
        }
    }
}


static const function_calls_t ibr_calls = {
        .interface_handle_size = sizeof(irb_setup_t),
        .init = ibr_init,
        .init_outputs = ibr_init_outputs,
        .init_inputs = ibr_init_inputs,
        .set_params = ibr_set_params,
        .pre_exec_init = NULL,
        .exec = ibr_exec
};

void ibr_get_handler(ibr_cfg_t *ibr_cfg, function_handler_t *fh) {
    fh->spec = &ibr_cfg->spec;
    fh->extension_handler = ibr_cfg;
    fh->calls = &ibr_calls;
}
