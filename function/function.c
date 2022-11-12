#include "function.h"

const char *fspec_find_path2connect(const func_conn_spec_t *conn_spec, const char *in_alias) {
    if (conn_spec != NULL) {
        for (int i = 0; conn_spec[i].alias != NULL; i++) {
            if (strcmp(conn_spec[i].alias, in_alias) == 0) {
                return conn_spec[i].value;
            }
        }
    }
    return NULL;
}

const char *fspec_find_param(const func_param_t *params, const char *p_alias) {
    return fspec_find_path2connect(params, p_alias);
}

const connection_spec_t *fspec_conn_find(const connection_spec_t **conn_spec, const char *alias2find) {
    int i;

    if (conn_spec != NULL) {
        for (i = 0; conn_spec[i] != NULL; i++) {
            if (strcmp(alias2find, conn_spec[i]->name) == 0) {
                return conn_spec[i];
            }
        }
    }

    return NULL;
}

int fspec_conn_arr_size(const connection_spec_t **conn_spec) {
    int rv = 0;
    if (conn_spec != NULL) {
        for (rv = 0; conn_spec[rv] != NULL; rv++);
    }
    return rv;
}

function_flow_t *flow_registry[16] = {
    NULL
};

const function_flow_t *fspec_find_flow(const char *spec_name) {
    function_handler_t *rv;

    for (int i = 0; flow_registry[i] != NULL; i++) {
        if (strcmp(flow_registry[i]->spec.name, spec_name) == 0) {
            return flow_registry[i];
        }
    }

    return NULL;
}

#include <stdlib.h>

void *function_alloc(size_t s) {
    return calloc(1, s);
}

fspec_rv_t function_alloc_handle(const function_handler_t *fh, void **dhandle) {
    size_t ifs = fh->calls->interface_handle_size;

    if ( ifs > 0) {
        *dhandle = function_alloc(ifs);
        if (*dhandle == NULL) {
            return fspec_rv_no_memory;
        }
    }

    return fspec_rv_ok;
}

fspec_rv_t function_init(const function_handler_t *fh, const char *inv_name,
                         eswb_topic_descr_t mounting_td, void **dhandle) {
    if (*dhandle == NULL) {
        fspec_rv_t rv = function_alloc_handle(fh, dhandle);
        if (rv != fspec_rv_ok) {
            return rv;
        }
    }

    if (fh->calls->init != NULL) {
        return fh->calls->init(*dhandle, fh->spec, inv_name, mounting_td, fh->extension_handler);
    } else {
        return fspec_rv_not_supported;
    }
}

fspec_rv_t function_init_inputs(const function_handler_t *fh, void *interface, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_td){
    if (fh->calls->init_inputs != NULL) {
        return fh->calls->init_inputs(interface, conn_spec, mounting_td);
    } else {
        return fspec_rv_not_supported;
    }
}

fspec_rv_t function_init_outputs(const function_handler_t *fh, void *interface, const func_conn_spec_t *conn_spec,
                                 eswb_topic_descr_t mounting_td, const char *func_name) {
    if (fh->calls->init_outputs != NULL) {
        return fh->calls->init_outputs(interface, conn_spec, mounting_td, func_name);
    } else {
        return fspec_rv_not_supported;
    }
}

fspec_rv_t
function_set_param(const function_handler_t *fh, void *interface, const func_param_t *params, int initial_call) {
    if (fh->calls->set_params != NULL) {
        return fh->calls->set_params(interface, params, initial_call);
    } else {
        return fspec_rv_not_supported;
    }
}

fspec_rv_t function_pre_exec_init(const function_handler_t *fh, void *interface) {
    if (fh->calls->pre_exec_init != NULL) {
        return fh->calls->pre_exec_init(interface);
    } else {
        return fspec_rv_not_supported;
    }
}

void function_exec(const function_handler_t *fh, void *interface){
    fh->calls->exec(interface);
}

fspec_rv_t function_gettime(func_time_t ft, struct timespec *t) {
    clockid_t id;
    switch (ft) {
        case ft_realtime:
            id = CLOCK_REALTIME;
            break;

        default:
        case ft_monotonic:
        case ft_monotonic_prec:
            id = CLOCK_MONOTONIC;
            break;
    }
    int rv = clock_gettime(id, t);
    return rv ? fspec_rv_invtime : fspec_rv_ok;
}

fspec_rv_t function_getdeltatime(func_time_t ft, struct timespec *prev, deltatime_policy_t p, double *dt) {
#   define DELTA_T(a,b) ( ( (a)->tv_sec - (b)->tv_sec ) + ( (double) ( (a)->tv_nsec - (b)->tv_nsec ) / 1000000000 ) )
    struct timespec curr;
    fspec_rv_t rv = function_gettime(ft, &curr);
    if (rv != fspec_rv_ok) {
        return rv;
    }

    *dt = DELTA_T(&curr, prev);

    if (p == dtp_update_prev) {
        *prev = curr;
    }

    return fspec_rv_ok;
}



const char *fspec_errmsg(fspec_rv_t c) {
    switch (c) {
        case fspec_rv_ok:               return "OK";
        case fspec_rv_no_update:        return "No update";
        case fspec_rv_inval_param:      return "Invalid parameters";
        case fspec_rv_no_memory:        return "Not enough memory";
        case fspec_rv_exists:           return "Entity exists";
        case fspec_rv_not_supported:    return "Not supported";
        case fspec_rv_no_input:         return "No such input";
        case fspec_rv_no_param:         return "No such param";
        case fspec_rv_no_topic:         return "No such topic";
        case fspec_rv_no_path:          return "Path is not specified";
        case fspec_rv_publish_err:      return "Publishing error";
        case fspec_rv_empty:            return "Flow is empty";
        case fspec_rv_invarg:           return "Invalid argument";
        case fspec_rv_loaderr:          return "Load error";
        case fspec_rv_initerr:          return "Init error";
        default:                        return "Unknown error";
    }
}

#include "flow.h"

const function_handler_t* function_lookup_declared(const char *spec_name) {
    function_handler_t *fh;
    fspec_rv_t rv;

    rv = flow_reg_find_handler(spec_name, &fh);
    if (rv == fspec_rv_ok) {
        return fh;
    }

    return NULL;
}
