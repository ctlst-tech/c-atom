#include <eswb/api.h>
#include "function.h"
#include "ibr_process.h"
#include "ibr_private.h"

void *ibr_alloc(size_t s);

ibr_msg_t *ibr_protocol_find_msg(const protocol_t *protocol, const char *msg_name) {
    for(int i = 0; protocol->msgs[i] != NULL; i++) {
        if (strcmp(protocol->msgs[i]->name, msg_name) == 0) {
            return protocol->msgs[i];
        }
    }

    return NULL;
}

static int mtd_is_function(irb_media_driver_type_t mdt) {
    return mdt == mdt_function || mdt == mdt_function_bridge;
}

static ibr_rv_t setup_connections(ibr_cfg_t *ibr_cfg, process_cfg_t *proc_cfg, irb_process_setup_t *setup, const function_spec_t *spec) {
    ibr_rv_t rv;

    irb_media_driver_type_t mdt_src;
    irb_media_driver_type_t mdt_dst;

    rv = ibr_decode_addr(proc_cfg->src, &mdt_src, &setup->src.access_addr);
    if (rv !=ibr_ok) {
        dbg_msg("Invalid source (%s) specification for IBR \"%s\"", proc_cfg->src, ibr_cfg->spec.name);
        return rv;
    }

    rv = ibr_decode_addr(proc_cfg->dst, &mdt_dst, &setup->dst.access_addr);
    if (rv !=ibr_ok) {
        dbg_msg("Invalid dest (%s) specification for IBR \"%s\"", proc_cfg->src, ibr_cfg->spec.name);
        return rv;
    }

    setup->src.mdt = mdt_src;
    setup->dst.mdt = mdt_dst;

    setup->src.drv = ibr_get_driver (mdt_src);
    setup->dst.drv = ibr_get_driver (mdt_dst);

    if (setup->src.drv == NULL || setup->dst.drv == NULL) {
        dbg_msg("Drivers setup error for IBR \"%s\"", ibr_cfg->spec.name);
        return ibr_nomedia;
    }

    if (mtd_is_function(mdt_dst)) {
        const connection_spec_t *cs = fspec_conn_find(spec->outputs, setup->dst.access_addr);
        if (cs == NULL) {
            dbg_msg("There is no output \"%s\" referenced by dst of \"%s\"",
                    setup->dst.access_addr,
                    proc_cfg->name);
            return ibr_invarg;
        }
    }

    if (mtd_is_function(mdt_src)) {
        const connection_spec_t *cs = fspec_conn_find(spec->inputs, setup->src.access_addr);
        if (cs == NULL) {
            dbg_msg("There is no input \"%s\" referenced by src of \"%s\"",
                    setup->src.access_addr,
                    proc_cfg->name);
            return ibr_invarg;
        }
    }

    return ibr_ok;
}

//static ibr_rv_t ibr_process_msg_setup(ibr_cfg_t *ibr_cfg, const function_spec_t *spec,
//                                      process_cfg_t *proc_cfg, irb_process_setup_t *setup) {
//
//    setup->name = proc_cfg->name;
//
//    setup->dst_msg = ibr_protocol_find_msg(&ibr_cfg->protocol, proc_cfg->msg);
//    if (setup->dst_msg == NULL) {
//        dbg_msg("No message \"%s\" for IBR \"%s\"", proc_cfg->msg, ibr_cfg->spec.name);
//        return ibr_noent;
//    }
//
//    // TODO compile message conversion instructions batch
//
//    return setup_connections(ibr_cfg, proc_cfg,setup, spec);
//}

ibr_process_type_t ibr_process_type_parse(const char *type_str) {
    if (strcmp(type_str, "frame") == 0) {
        return ibr_process_type_frame;
    } else if (strcmp(type_str, "copy") == 0) {
        return ibr_process_type_copy;
    }
    return ibr_process_type_invalid;
}

static ibr_rv_t ibr_process_frame_setup(ibr_cfg_t *ibr_cfg, const function_spec_t *spec,
                                        protocol_t *protocol,
                                        process_cfg_t *proc_cfg, irb_process_setup_t *setup) {

    ibr_rv_t rv;

    setup->name = proc_cfg->name;

    setup->type = ibr_process_type_parse(proc_cfg->type);
    if (setup->type == ibr_process_type_invalid) {
        dbg_msg("Invalid IBR process type: %s", proc_cfg->type);
    }

    switch (setup->type) {
        case ibr_process_type_frame:
            setup->frame = protocol->frame;

            setup->msgs_num = protocol->frame->msg_num;
            setup->msgs_setup = ibr_alloc(sizeof(*setup->msgs_setup) * setup->msgs_num);
            if (setup->msgs_setup == NULL) {
                return ibr_nomem;
            }

            for (int i = 0; i < setup->msgs_num; i++) {
                setup->msgs_setup[i].src_msg = setup->frame->msgs[i];
                setup->msgs_setup[i].id = setup->frame->msgs[i]->id;

                rv = ibr_msg_to_functional_msg(
                        setup->msgs_setup[i].src_msg,
                        &setup->msgs_setup[i].dst_msg,
                        &setup->msgs_setup[i].conv_queue);
                if (rv != ibr_ok) {
                    return rv;
                }
            }

            break;

        case ibr_process_type_copy:
            setup->frame = NULL;
            setup->msgs_num = 1;
            setup->msgs_setup = ibr_alloc(sizeof(*setup->msgs_setup));
            break;

        default:
            return ibr_invarg;
    }

    // TODO setup frame
    // TODO compile message conversion instructions batch

    return setup_connections(ibr_cfg, proc_cfg, setup, spec);
}




fspec_rv_t ibr_init(void *dhandle, const function_spec_t *spec, const char *inv_name, eswb_topic_descr_t mounting_td, const void *extension_handler) {
    int err_cnt = 0;
    ibr_cfg_t *ibr_cfg = (ibr_cfg_t *) extension_handler;
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    ibr_rv_t rv;

    ibr_setup->process_setups = ibr_alloc(ibr_cfg->processes_num * sizeof(irb_process_setup_t));
    ibr_setup->processes_num = ibr_cfg->processes_num;

    for (int i = 0; i < ibr_cfg->processes_num; i++) {
        rv = ibr_process_frame_setup(ibr_cfg, spec,
                                     ibr_cfg->protocol,
                                     &ibr_cfg->processes[i], &ibr_setup->process_setups[i]);
        if (rv != ibr_ok) {
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

static ibr_rv_t connect_output(irb_process_setup_t *process_setup, const func_conn_spec_t *conn_spec, msg_record_t *msg_r) {
    int err_cnt = 0;
    ibr_rv_t rv;

    if (process_setup->dst.drv->proclaim != NULL) {
        const char *eswb_out_path = fspec_find_path2connect(conn_spec, process_setup->dst.access_addr);
        if (eswb_out_path == NULL) {
            dbg_msg("Output \"%s\" have no specified connection", process_setup->dst.access_addr);
            err_cnt++;
            return ibr_invarg;
        }

        rv = process_setup->dst.drv->proclaim(eswb_out_path, NULL,
                                                            msg_r->dst_msg,
                                                            &msg_r->descr);
    } else {
        rv = process_setup->dst.drv->connect(process_setup->dst.access_addr,
                                                           &process_setup->dst.descr);
    }

    return rv;
}

fspec_rv_t ibr_init_outputs(void *dhandle, const func_conn_spec_t *conn_spec,
                             eswb_topic_descr_t mounting_point, const char *func_name) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    int err_cnt = 0;

    ibr_rv_t rv = ibr_noent;

    for (int i = 0; i < ibr_setup->processes_num; i++) {
        for (int j = 0; j < ibr_setup->process_setups[i].msgs_num; j++) {
            rv = connect_output(&ibr_setup->process_setups[i], conn_spec, &ibr_setup->process_setups[i].msgs_setup[j]);
            if (rv != ibr_ok) {
                err_cnt++;
            }
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}


fspec_rv_t ibr_init_inputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point) {
    irb_setup_t *ibr_setup = (irb_setup_t *) dhandle;
    int err_cnt = 0;
    ibr_rv_t rv;

    for (int i = 0; i < ibr_setup->processes_num; i++) {

        const char *in_path;

        if (ibr_setup->process_setups[i].src.mdt == mdt_function ||
                ibr_setup->process_setups[i].src.mdt == mdt_function_bridge ||
                ibr_setup->process_setups[i].src.mdt == mdt_function_vector) {
            in_path = fspec_find_path2connect(conn_spec, ibr_setup->process_setups[i].src.access_addr);
            if (in_path == NULL) {
                dbg_msg("Input \"%s\" have no specified connection", ibr_setup->process_setups[i].src.access_addr);
                err_cnt++;
                continue;
            }
        } else {
            in_path = ibr_setup->process_setups[i].src.access_addr;
        }

//        if (ibr_setup->process_setups[i].src.drv->proclaim != NULL) {
//            ibr_msg_t *msg = ibr_alloc(sizeof(ibr_msg_t));
//            if (msg == NULL) {
//                return fspec_rv_no_memory;
//            }
//            msg->name = conn_spec->alias;
//
//            ibr_setup->process_setups[i].src_msg = msg;
//
//            rv = ibr_setup->process_setups[i].src.drv->proclaim(in_path,
//                                                                ibr_setup->process_setups[i].src_msg,
//                                                                ibr_setup->process_setups[i].dst_msg,
//                                                                &ibr_setup->process_setups[i].src.descr);
//        } else {
            rv = ibr_setup->process_setups[i].src.drv->connect(in_path,
                                                               &ibr_setup->process_setups[i].src.descr);
//        }

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

    ibr_process_thread(ibr_setup->process_setups);

//    for (int i = 0; i < ibr_setup->processes_num; i++) {
//        rv = ibr_process_start(&ibr_setup->process_setups[i]);
//        if (rv != ibr_ok) {
//            dbg_msg("ibr_process_start failed");
//        }
//    }
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
