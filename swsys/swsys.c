//
// Created by goofy on 2/2/22.
//

#include <stdlib.h>
#include <unistd.h>

#include <pthread.h> // FIXME

#include <eswb/api.h>
#include <eswb/event_queue.h>

#include "swsys.h"
#include "function.h"
#include "fsminst.h"
#include "flow.h"
#include "ibr.h"

typedef struct {
    const swsys_task_t *task;
    eswb_topic_descr_t clk_input_td;
    pthread_t tid; // FIXME must be os independent
} swsys_task_handle_t;

typedef struct {
    const swsys_t *swsys;
    eswb_topic_descr_t *clk_input_tds;
} swsys_func_dhandle_t;

void *swsys_alloc(size_t s);

static swsys_rv_t task_load(swsys_task_t *t) {
    swsys_rv_t rv = swsys_e_ok;

    switch (t->type) {
        case tsk_flow:
            ;
            function_flow_t *ff = NULL;
            fspec_rv_t flow_rv = flow_load(t->config_path, &ff);
            if (flow_rv != fspec_rv_ok) {
                rv = swsys_e_loaderr;
            } else {
                flow_get_handler(ff, &t->func_handler);
            }
            break;

        case tsk_fsm:
            ;
            fsminst_t *fsminst = NULL;
            fsm_rv_t fsm_rv = fsminst_load(t->config_path, &fsminst);
            if (fsm_rv != fsm_rv_ok) {
                rv = swsys_e_loaderr;
            } else {
                fsminst_get_handler(fsminst, &t->func_handler);
            }
            break;

        case tsk_ibr:
            ;
            ibr_cfg_t *ibr_cfg = NULL;
            ibr_rv_t ibr_rv = ibr_cfg_load(t->config_path, &ibr_cfg);
            if (ibr_rv != ibr_ok) {
                rv = swsys_e_loaderr;
            } else {
                ibr_get_handler(ibr_cfg, &t->func_handler);
            }
            t->exec_once = -1;
            break;

        default:
            rv = swsys_e_invargs;
            break;
    }


    return rv;
}

static swsys_rv_t swsys_init(swsys_t *sys) {
    int i;
    int err_num = 0;
    swsys_rv_t rv;
    eswb_rv_t erv;

    for (i = 0; i < sys->busses_num; i++) {
        eswb_topic_descr_t bus_td = 0;
        char path[ESWB_TOPIC_MAX_PATH_LEN + 1];

        // TODO make a difference for bus type depending on a init context: system, thread
        eswb_type_t bus_type = eswb_inter_thread; // for now, while we operate w/o brokers
        erv = eswb_create(sys->busses[i].name, bus_type,sys->busses[i].max_topics);
        if (erv != eswb_e_ok) {
            dbg_msg("ESWB bus creation error: %s", eswb_strerror(erv));
            err_num++;
        }

        erv = eswb_path_compose(bus_type, sys->busses[i].name, NULL, path);
        if (erv != eswb_e_ok) {
            dbg_msg("ESWB eswb_path_compose error: %s", eswb_strerror(erv));
            err_num++;
        }

        erv = eswb_topic_connect(path, &bus_td);
        if (erv != eswb_e_ok) {
            dbg_msg("ESWB bus creation error: %s", eswb_strerror(erv));
            err_num++;
        }

        if ((sys->busses[i].evq_size > 0) && (sys->busses[i].evq_buffer_size > 0)) {
            erv = eswb_event_queue_enable(bus_td, sys->busses[i].evq_size, sys->busses[i].evq_buffer_size);
            if (erv != eswb_e_ok) {
                dbg_msg("ESWB event queue enable error: %s", eswb_strerror(erv));
                err_num++;
            }
        }

        if (sys->busses[i].eq_channel) {
            erv = eswb_event_queue_order_topic(bus_td, sys->busses[i].name, sys->busses[i].eq_channel);
            if (erv != eswb_e_ok) {
                dbg_msg("ESWB eswb_event_queue_order_topic error: %s", eswb_strerror(erv));
                err_num++;
            }
        }

        if (sys->busses[i].dirs != NULL) {
            for (int j = 0; sys->busses[i].dirs[j].path != NULL; j++) {
                erv = eswb_path_compose(bus_type, sys->busses[i].name, sys->busses[i].dirs[j].path, path);
                if (erv != eswb_e_ok) {
                    return swsys_e_invargs;
                }
                erv = eswb_mkdir(path, NULL);
                if (erv != eswb_e_ok) {
                    dbg_msg("ESWB dir \"%s\" creation error: %s", path, eswb_strerror(erv));
                    err_num++;
                } else {
                    if (sys->busses[i].dirs[j].eq_channel) {
                        sprintf(path, "%s/%s", sys->busses[i].name, sys->busses[i].dirs[j].path);
                        erv = eswb_event_queue_order_topic(bus_td, path, sys->busses[i].dirs[j].eq_channel);
                        if (erv != eswb_e_ok) {
                            dbg_msg("ESWB eswb_event_queue_order_topic error: %s", eswb_strerror(erv));
                            err_num++;
                        }
                    }
                }
            }
        }
    }

    for (i = 0; i < sys->tasks_num; i++) {
        rv = task_load(&sys->tasks[i]);
        if (rv == swsys_e_ok) {
            fspec_rv_t frv;
            frv = function_init(&sys->tasks[i].func_handler, sys->tasks[i].name, &sys->tasks[i].func_call_dhandle);
            if (frv != fspec_rv_ok){
                dbg_msg_ec(frv, "Task \"%s\" function_init error", sys->tasks[i].name);
                err_num++;
            }
        } else {
            dbg_msg("Task \"%s\" load error (%d)", sys->tasks[i].name, rv);
            err_num++;
        }
    }

    return err_num > 0 ? swsys_e_loaderr : swsys_e_ok;
}

static fspec_rv_t swsys_call_init (void *dhandle, const function_spec_t *spec, const char *inv_name, const void *extension_handler) {
    swsys_func_dhandle_t *ssdh = (swsys_func_dhandle_t *)dhandle;

    ssdh->swsys = (const swsys_t *) extension_handler;
    swsys_rv_t srv = swsys_init((swsys_t *)ssdh->swsys);

    if (srv == swsys_e_ok) {
        ssdh->clk_input_tds = swsys_alloc(ssdh->swsys->tasks_num);
        if (ssdh->clk_input_tds == NULL) {
            return fspec_rv_no_memory;
        }
    }

    return srv != swsys_e_ok ? fspec_rv_initerr : fspec_rv_ok;
}

static fspec_rv_t swsys_call_init_inputs (void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_td) {
    swsys_func_dhandle_t *ssdh = (swsys_func_dhandle_t *)dhandle;
    const swsys_t *swsys = ssdh->swsys;

    int err_cnt = 0;
    for (int i = 0; i < swsys->tasks_num; i++) {
        mounting_td = mounting_td; // TODO handle overall connectivity

        if (swsys->tasks[i].clk_method == swsys_clk_inp_upd) {
            eswb_rv_t erv = eswb_subscribe(swsys->tasks[i].clk_input_path,
                                           &ssdh->clk_input_tds[i]
                                           );
            if (erv != eswb_e_ok) {
                dbg_msg("Clocking input subscription error: %s", eswb_strerror(erv));
                err_cnt++;
            }
        }

        fspec_rv_t rv = function_init_inputs(&swsys->tasks[i].func_handler,
                                             swsys->tasks[i].func_call_dhandle,
                                             swsys->tasks[i].conn_specs_in,
                                             mounting_td);
        
        if (rv != fspec_rv_ok) {
            dbg_msg_ec(rv, "function_init_inputs failed for task \"%s\"", swsys->tasks[i].name);
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

static fspec_rv_t swsys_call_init_outputs (void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_td, const char *func_name) {
    swsys_func_dhandle_t *ssdh = (swsys_func_dhandle_t *)dhandle;
    const swsys_t *swsys = ssdh->swsys;

    int err_cnt = 0;
    for (int i = 0; i < swsys->tasks_num; i++) {
        mounting_td = mounting_td; // TODO handle overall connectivity
        // TODO likely there will be outputs call init refactoring
        fspec_rv_t rv = function_init_outputs(&swsys->tasks[i].func_handler,
                                              swsys->tasks[i].func_call_dhandle,
                                              swsys->tasks[i].conn_specs_out,
                                              mounting_td, NULL);
        if (rv != fspec_rv_ok) {
            dbg_msg_ec(rv, "function_init_outputs failed for %s task", swsys->tasks[i].name);
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

static fspec_rv_t swsys_call_set_param(void *dhandle, const func_param_t *params, int initial_call) {
    swsys_func_dhandle_t *ssdh = (swsys_func_dhandle_t *)dhandle;
    const swsys_t *swsys = ssdh->swsys;

    int err_cnt = 0;

    // TODO process own params

    for (int i = 0; i < swsys->tasks_num; i++) {
        fspec_rv_t rv = function_set_param(&swsys->tasks[i].func_handler, swsys->tasks[i].func_call_dhandle,
                                           swsys->tasks[i].params, initial_call);
        if (rv != fspec_rv_ok) {
            err_cnt++;
        }
    }

    return err_cnt > 0 ? fspec_rv_initerr : fspec_rv_ok;
}

void eswb_set_thread_name(const char *n);

static void catom_set_pthread_name(swsys_task_type_t tt, const char *tn) {
    char tname[16];

    switch (tt) {
        case tsk_flow:  strcpy(tname, "flw-"); break;
        case tsk_fsm:   strcpy(tname, "fsm-"); break;
        case tsk_ibr:   strcpy(tname, "ibr-"); break;
        default:        strcpy(tname, "arb-"); break;
    }

    strncat(tname, tn, sizeof(tname) - 4 - 1);
    eswb_set_thread_name(tname);
}

void ibr_set_pthread_name(const char *tn) {
    catom_set_pthread_name(tsk_ibr, tn);
}

static void* task(void *taskhndl) {
    int loop = 1;

    swsys_task_handle_t * th = (swsys_task_handle_t *) taskhndl;
    uint32_t delay = th->task->clk_period_ms * 1000;

    catom_set_pthread_name(th->task->type, th->task->name);

    while(loop) {
        switch (th->task->clk_method) {
            case swsys_clk_freerun:
                break;

            case swsys_clk_timer:
                usleep(delay); // FIXME use best OS option to clock control
                break;

            case swsys_clk_inp_upd:
                ;
                eswb_rv_t rv = eswb_get_update(th->clk_input_td, NULL);
                if (rv != eswb_e_ok) {
                    dbg_msg("eswb_get_update failed for task \"%s\": %s", th->task->name, eswb_strerror(rv));
                    loop = 0;
                    continue;
                }
                break;

            default:
            case swsys_clk_ext_call:
                loop = 0;
                continue;
        }

        function_exec(&th->task->func_handler, th->task->func_call_dhandle);
//        dbg_msg("Updating %s", th->task->name);
    }
    
    return NULL;
}

int start_task(swsys_task_handle_t *th){

    // TODO handle priority
    // TODO make OS independent wrapper

    int rv = pthread_create(&th->tid, NULL, task, th);
    if (rv != 0) {
        dbg_msg("pthread_create failed: %s", strerror(rv));
    }

    return rv;
}

int swsys_service_start(const swsys_service_t *s);

static void swsys_call_exec (void *dhandle) {
    swsys_func_dhandle_t *ssdh = (swsys_func_dhandle_t *)dhandle;

    int i;

    for (i = 0; i < ssdh->swsys->services_num; i++) {
        int rv = swsys_service_start(&ssdh->swsys->services[i]);
        if (rv != 0) {
            dbg_msg("swsys_service_start \"%s\" failed", ssdh->swsys->services[i].name);
            return;
        }
    }

    for (i = 0; i < ssdh->swsys->tasks_num; i++) {
        swsys_task_handle_t *th = swsys_alloc(sizeof (*th));
        if (th == NULL) {
            dbg_msg("swsys_alloc failed");
            return;
        }

        th->task = &ssdh->swsys->tasks[i];
        th->clk_input_td = ssdh->clk_input_tds[i];

        if (!th->task->exec_once) {
            if (i < ssdh->swsys->tasks_num - 1) {
                int rv = start_task(th);
                if (rv != 0) {
                    return;
                }
            } else {
                // calling thread runs tha last task
                task(th);
            }
        } else {
            // FIXME if this task is last, process terminates
            function_exec(&th->task->func_handler, th->task->func_call_dhandle);
        }
    }
}

static const function_calls_t swsys_calls = {
    .interface_handle_size = sizeof(swsys_func_dhandle_t),
    .init = swsys_call_init,
    .init_outputs = swsys_call_init_outputs,
    .init_inputs = swsys_call_init_inputs,
    .set_params = swsys_call_set_param,
    .exec = swsys_call_exec
};


void swsys_get_handler(swsys_t *swsys, function_handler_t *fh) {
    fh->spec = NULL; // TODO is it ok for top system?
    fh->extension_handler = swsys;
    fh->calls = &swsys_calls;
}


swsys_rv_t swsys_top_module_start(swsys_t *swsys) {

    function_handler_t fh;
    fspec_rv_t rv;

    swsys_get_handler(swsys, &fh);

    swsys_func_dhandle_t *dhandle = NULL;

    rv = function_init(&fh, NULL, (void **) &dhandle);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "function_init failed");
        return swsys_e_initerr;
    }

    rv = function_set_param(&fh, dhandle, NULL, 1);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "function_set_param failed");
        return swsys_e_initerr;
    }

    rv = function_init_outputs(&fh, dhandle, NULL, 0, NULL);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "function_init_outputs failed");
        return swsys_e_initerr;
    }

    rv = function_init_inputs(&fh, dhandle, NULL, 0);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "function_init_inputs failed");
        return swsys_e_initerr;
    }

    function_exec(&fh, dhandle);

    return swsys_e_ok;
}
