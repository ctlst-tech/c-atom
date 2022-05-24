
#include <stdlib.h>
#include <eswb/api.h>
#include <eswb/errors.h>

#include "fsminst.h"
#include "function.h"
#include "fsm.h"
#include "symbol.h"

typedef struct {
    const connection_spec_t *conn_spec;
    symbol_t *symbol;
    eswb_topic_descr_t td;
} fsminst_connector_t;

typedef struct {
    fsm_t **fsms;
    symbol_context_t symbols;

    fsminst_connector_t *inputs;
    fsminst_connector_t *outputs;
} fsm_dhandle_t;


void *fsminst_alloc(size_t s) {
    return calloc(1, s);
}

static result_type_t eswb2fsm_type(topic_data_type_t rt);

static fspec_rv_t symbols_add_spec(symbol_context_t *symb_context,
                                   const connection_spec_t **conn_spec,
                                   fsminst_connector_t **connectors_rv) {
    fsminst_connector_t *connectors;
    int conns_cnt = fspec_conn_arr_size(conn_spec);
    int einval_cnt = 0;

    if (conns_cnt == 0) {
        return fspec_rv_ok;
    }

    if (connectors_rv != NULL) {
        connectors = fsminst_alloc((1 + conns_cnt) * sizeof(fsminst_connector_t));
        if (connectors == NULL) {
            return fspec_rv_no_memory;
        }
    }

    int i;
    for (i = 0; conn_spec[i] != NULL; i++) {
        // TODO think about adding prefixes | suffixes
        if (symbol_find(symb_context, conn_spec[i]->name) != NULL) {
            return fspec_rv_exists;
        }

        result_type_t rt = eswb2fsm_type(conn_spec[i]->type);
        if (rt == nr_na) {
            dbg_msg("Not supported type for fsm %d", conn_spec[i]->type);
            einval_cnt++;
        }
        symbol_t *s = symbol_add_var(symb_context, conn_spec[i]->name, rt);
        if (s == NULL) {
            // TODO free other resources?
            return fspec_rv_no_memory;
        }

        if (conn_spec[i]->default_value != NULL) {
            int rv = symbol_update_w_str(s, conn_spec[i]->default_value);
            if (rv) {
                // TODO report msg
                einval_cnt++;
            }
        }

        if (connectors_rv != NULL) {
            connectors[i].symbol = s;
            connectors[i].td = 0;
            connectors[i].conn_spec = conn_spec[i];
        }
    }

    if (connectors_rv != NULL) {
        connectors[i].conn_spec = NULL;
        *connectors_rv = connectors;
    }

    return einval_cnt > 0 ? fspec_rv_inval_param : fspec_rv_ok;
}

int eint_func_timeout_construct(void *dhandle, void **state_ref) {
    void *rv = calloc(1, sizeof(struct timespec));
    if (rv == NULL) {
        return 1;
    }
    *state_ref = rv;
    return 0;
}

int eint_func_timeout_actiavte(void *state_ref) {
    struct timespec *activation_time = state_ref;
    function_gettime(ft_monotonic, activation_time);
    return 0;
}

static void eint_func_timeout (symbol_func_args_list_t *args, result_val_t *rv) {
    struct timespec *activation_time = args->args[0]->val.p;

    double dt;
    function_getdeltatime(ft_monotonic, activation_time, dtp_none, &dt);

    rv->b = dt >= args->args[1]->val.f ? TRUE : FALSE;
}

struct trueduring_state {
    struct timespec arm_time;
    int armed;
};

int eint_func_trueduring_construct(void *dhandle, void **state_ref) {
    void *rv = calloc(1, sizeof(struct trueduring_state));
    if (rv == NULL) {
        return 1;
    }
    *state_ref = rv;
    return 0;
}

int eint_func_trueduring_actiavte(void *state_ref) {
    struct trueduring_state *state = state_ref;
    state->armed = 0;
    return 0;
}


static void eint_func_trueduring (symbol_func_args_list_t *args, result_val_t *rv) {
    struct trueduring_state *state = args->args[0]->val.p;

    double dt;
    boolval_t result = FALSE;

    if (args->args[1]->val.b) {
        if (state->armed) {
            function_getdeltatime(ft_monotonic, &state->arm_time, dtp_none, &dt);
            if (dt >= args->args[2]->val.f) {
                result = TRUE;
            }
        } else {
            function_gettime(ft_monotonic, &state->arm_time);
            state->armed = -1;
        }
    } else {
        state->armed = 0;
    }

    rv->b = result;
}

static symbol_state_handler_t timeout_sh = {
        .dhandle = NULL,
        .state_constructor = eint_func_timeout_construct,
        .state_activate = eint_func_timeout_actiavte
};

static symbol_state_handler_t trueduring_sh = {
        .dhandle = NULL,
        .state_constructor = eint_func_trueduring_construct,
        .state_activate = eint_func_trueduring_actiavte
};

fspec_rv_t fsm_init_call(void *iface, const function_spec_t *spec, const char *inv_name, const void *extension_handler) {
    fsm_dhandle_t *dfsm = iface;
    dfsm->fsms = (fsm_t **) extension_handler;

    fspec_rv_t rv;
    rv = symbols_add_spec(&dfsm->symbols, spec->inputs, &dfsm->inputs);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "symbols_add_spec for inputs of %s", spec->name);
        return rv;
    }

    rv = symbols_add_spec(&dfsm->symbols, spec->outputs, &dfsm->outputs);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "symbols_add_spec for outputs of %s", spec->name);
        return rv;
    }

    rv = symbols_add_spec(&dfsm->symbols, spec->params, NULL);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "symbols_add_spec for params of %s", spec->name);
        return rv;
    }

    rv = symbols_add_spec(&dfsm->symbols, spec->state_vars, NULL);
    if (rv != fspec_rv_ok) {
        dbg_msg_ec(rv, "symbols_add_spec for vars of %s", spec->name);
        return rv;
    }

    symbol_add_func(&dfsm->symbols, "timeout", eint_func_timeout, &timeout_sh, 1, nr_bool);
    symbol_add_func(&dfsm->symbols, "trueduring", eint_func_trueduring, &trueduring_sh, 2, nr_bool);

    for (int i = 0; dfsm->fsms[i] != NULL; i++) {
        fsm_attach_symbol_context(dfsm->fsms[i], &dfsm->symbols);
        fsm_rv_t frv = fsm_init_and_compile(dfsm->fsms[i]);
        if (frv != fsm_rv_ok) {
            dbg_msg("fsm_init_and_compile error for %s", spec->name);
            return fspec_rv_initerr;
        }
    }

    return fspec_rv_ok;
}

fspec_rv_t fsm_set_params(void *dhandle, const func_param_t *params, int initial_call) {
    fsm_dhandle_t *dfsm = dhandle;
    int err_num_no_symb = 0;
    int err_num_inv_format = 0;

    if (params == NULL) {
        return fspec_rv_ok;
    }

    for (int i = 0; params[i].alias != NULL; i++) {
        symbol_t *s = symbol_find(&dfsm->symbols, params[i].alias);
        if (s == NULL) {
            dbg_msg("Symbol \"%s\"| is not found", params[i].alias);
            err_num_no_symb++;
        } else {
            if (symbol_update_w_str(s, params[i].value) ) {
                dbg_msg("Symbol \"%s\" update by invalid format \"%s\"", params[i].alias, params[i].value);
                err_num_inv_format++;
            }
        }
    }

    fspec_rv_t rv;
    if (err_num_no_symb){
        rv = fspec_rv_no_param;
    } else if (err_num_inv_format) {
        rv = fspec_rv_inval_param;
    } else {
        rv = fspec_rv_ok;
    }

    return rv;
}

static topic_data_type_t fsm_type2eswb(result_type_t rt) {

    switch(rt) {
        case nr_float: return tt_double;
        case nr_int: return tt_int32;
        case nr_string: return tt_string; // TODO handle properly
        case nr_bool: return tt_int32;

        default:
        case nr_na:
        case nr_ptr:
            return tt_none;
    }
}

static result_type_t eswb2fsm_type(topic_data_type_t rt) {

    switch(rt) {
        case tt_double: return nr_float;
        case tt_int32: return nr_int;
        case tt_string: return nr_string;
//        case tt_int32: return nr_bool;

        default:
            return nr_na;
    }
}

fspec_rv_t fsm_init_outputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point, const char *func_name) {
    fsm_dhandle_t *dfsm = dhandle;
    int err_no_path_cnt = 0;
    int err_pub_cnt = 0;
    int err_type_cnt = 0;

    // TODO handle wildcard connection with publishing as vector
    // TODO align types and post array of

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 3);

    for (int i = 0; dfsm->outputs[i].conn_spec != NULL; i++) {
        const char *path = fspec_find_path2connect(conn_spec, dfsm->outputs[i].conn_spec->name);
        if (path != NULL) {
            topic_data_type_t type = fsm_type2eswb(dfsm->outputs[i].symbol->i.var.type);
            if (type != tt_none) {
                eswb_rv_t rv;
                char topic_path[ESWB_TOPIC_MAX_PATH_LEN + 1];
                char topic_name[ESWB_TOPIC_NAME_MAX_LEN + 1];
                rv = eswb_path_split(path, topic_path, topic_name);
                if (rv != eswb_e_ok) {
                    dbg_msg("Path of \"%s\" output has invalid format", dfsm->outputs[i].conn_spec->name);
                    return fspec_rv_invarg;
                }

                topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, topic_name,
                                                                  type, sizeof(result_val_t));

                rv = eswb_proclaim_tree_by_path(topic_path, rt, cntx->t_num, &dfsm->outputs[i].td);
                if (rv != eswb_e_ok) {
                    dbg_msg("ESWB proclaiming output \"%s\" to \"%s\" is failed: %s",
                            dfsm->outputs[i].conn_spec->name,
                            path,
                            eswb_strerror(rv)
                            );
                    err_pub_cnt++;
                }
            } else {
                dbg_msg("Type cast of output \"%s\" to ESWB failed", dfsm->outputs[i].conn_spec->name);
                err_type_cnt++;
            }
        } else {
            if (dfsm->outputs[i].conn_spec->flags.mandatory) {
                dbg_msg("Path is not specified for mandatory output \"%s\"", dfsm->outputs[i].conn_spec->name);
                err_no_path_cnt++;
            }
        }
    }

    if (err_pub_cnt) {
        return fspec_rv_publish_err;
    } else if (err_no_path_cnt) {
        return fspec_rv_no_path;
    } else if (err_type_cnt) {
        return fspec_rv_initerr;
    } else {
        return fspec_rv_ok;
    }
}


fspec_rv_t fsm_init_inputs(void *dhandle, const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_point) {
    fsm_dhandle_t *dfsm = dhandle;

    int err_no_path_cnt = 0;
    int err_subscr_cnt = 0;
    int err_type_cnt = 0;

    // TODO handle wildcard connection with subscribing as vector

    for (int i = 0; dfsm->inputs[i].conn_spec != NULL; i++) {
        const char *path = fspec_find_path2connect(conn_spec, dfsm->inputs[i].conn_spec->name);
        if (path != NULL) {
            eswb_rv_t rv = eswb_subscribe(path, &dfsm->inputs[i].td);
            if (rv != eswb_e_ok) {
                dbg_msg("Input \"%s\" subscribe failed: %s", dfsm->inputs[i].conn_spec->name, eswb_strerror(rv));
                err_subscr_cnt++;
            } else {
                topic_params_t p;
                rv = eswb_get_topic_params(dfsm->inputs[i].td, &p);
                if (rv != eswb_e_ok) {
                    dbg_msg("Getting ESWB params of Input \"%s\" failed: %s", dfsm->inputs[i].conn_spec->name, eswb_strerror(rv));
                    err_subscr_cnt++;
                } else {
                    // TODO check type widely, pick options for conversions
                    if ( p.type != fsm_type2eswb(dfsm->inputs[i].symbol->i.var.type)) {
                        dbg_msg("Casting ESWB type to Input \"%s\" failed", dfsm->inputs[i].conn_spec->name);
                        err_type_cnt++;
                    }
                }
            }
        } else {
            if (dfsm->inputs[i].conn_spec->flags.mandatory) {
                dbg_msg("Mandatory Input \"%s\" does not have specified path", dfsm->inputs[i].conn_spec->name);
                err_no_path_cnt++;
            }
        }
    }

    if (err_subscr_cnt) {
        return fspec_rv_no_topic;
    } else if (err_no_path_cnt) {
        return fspec_rv_no_path;
    } else if (err_type_cnt) {
        return fspec_rv_initerr;
    } else {
        return fspec_rv_ok;
    }
}


void fsm_exec(void *dhandle) {
    fsm_dhandle_t *dfsm = dhandle;
    eswb_rv_t erv;
    int i;

    uint8_t data[256]; // FIXME !!!! must be allocated as maximum of inputs sizes

    for (i = 0; dfsm->inputs[i].conn_spec != NULL; i++) {
        erv = eswb_read(dfsm->inputs[i].td, data);
        if (erv == eswb_e_ok) {
            symbol_update(dfsm->inputs[i].symbol, data);
        }
    }

    for (i = 0; dfsm->fsms[i] != NULL; i++) {
        fsm_do_update(dfsm->fsms[i]);
    }

    for (i = 0; dfsm->outputs[i].conn_spec != NULL; i++) {
        eswb_update_topic(dfsm->outputs[i].td, &dfsm->outputs[i].symbol->i.var.val);
    }
}

static function_calls_t fsminst_calls = {
    .interface_handle_size = sizeof(fsm_dhandle_t),
    .init = fsm_init_call,
    .init_outputs = fsm_init_outputs,
    .init_inputs = fsm_init_inputs,
    .set_params = fsm_set_params,
    .exec = fsm_exec
};


void fsminst_get_handler(fsminst_t *fsm, function_handler_t *fh) {
    fh->spec = &fsm->spec;
    fh->extension_handler = fsm->fsms;
    fh->calls = &fsminst_calls;
}
