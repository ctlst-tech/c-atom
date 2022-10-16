//
// Created by goofy on 5/31/21.
//

#ifndef ESWB_PROTO_MODULE_H
#define ESWB_PROTO_MODULE_H

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <eswb/types.h>


typedef struct topic_spec {
    const char *name;
    const char *annotation;
    const char *units;
    const char *default_value;

    topic_data_type_t type;

    struct {
        uint32_t mandatory:1;
    } flags;
} connection_spec_t;

typedef connection_spec_t input_spec_t;
typedef connection_spec_t output_spec_t;
typedef connection_spec_t param_spec_t;




typedef enum fspec_rv {
    fspec_rv_ok = 0,
    fspec_rv_no_update = 1,
    fspec_rv_inval_param,
    //fspec_rv_no_module_handler,
    fspec_rv_no_memory,
    fspec_rv_exists,
    //fspec_rv_modules_list_full,
    fspec_rv_not_exists,

    fspec_rv_invtime,
    fspec_rv_not_supported,
    fspec_rv_no_input,
    fspec_rv_no_param,
    fspec_rv_no_topic,
    fspec_rv_no_path,
    fspec_rv_publish_err,
    // fspec_rv_subscr_err,
    fspec_rv_empty,
    fspec_rv_invarg,
    fspec_rv_loaderr,
    fspec_rv_initerr,
} fspec_rv_t;

typedef struct func_pair {
    const char *alias;
    const char *value;
} func_pair_t;

typedef func_pair_t func_param_t;
typedef func_pair_t func_conn_spec_t;

struct function_spec;

typedef fspec_rv_t (*fspec_init_f)(void *interface, const struct function_spec *spec, const char *inv_name,
        eswb_topic_descr_t mounting_td, const void *extension_handler);
typedef fspec_rv_t (*fspec_init_inputs_f)(void *interface, const func_conn_spec_t *conn_spec,
        eswb_topic_descr_t mounting_td);
typedef fspec_rv_t (*fspec_init_outputs_f)(void *interface, const func_conn_spec_t *conn_spec,
        eswb_topic_descr_t mounting_td, const char *func_name);
typedef fspec_rv_t (*fspec_set_param_f)(void *interface, const func_param_t *params, int initial_call);
typedef void (*fspec_exec_f)(void *interface);


typedef struct function_spec {
    const char *name;
    const char *annotation;
    const input_spec_t **inputs;
    const output_spec_t **outputs;
    const param_spec_t **params;
    const param_spec_t **state_vars;
} function_spec_t;

typedef struct function_calls {
    const size_t interface_handle_size;

    const fspec_init_f init;

    const fspec_init_inputs_f init_inputs;
    const fspec_init_outputs_f init_outputs;

    const fspec_exec_f exec;
    const fspec_set_param_f set_params;
} function_calls_t;


typedef struct function_handler {
    const function_spec_t *spec;
    const function_calls_t *calls;

    const void *extension_handler; // e.g. funcs batch for flow
} function_handler_t;


typedef struct function_inside_flow {
    const char *name;
    const function_handler_t *h;
    const func_param_t *initial_params;
    func_conn_spec_t *connect_spec;
} function_inside_flow_t;

typedef struct function_flow_cfg {

    function_inside_flow_t *functions_batch;
    func_conn_spec_t       *outputs_links;

} function_flow_cfg_t;

typedef struct function_flow {

    function_spec_t spec;
    function_flow_cfg_t cfg;

} function_flow_t;

typedef struct function_defined {
    function_spec_t spec;
    void *handle;
} function_defined_t;

typedef enum func_time {
    ft_monotonic,
    ft_realtime,
    ft_monotonic_prec
}func_time_t;

const char *fspec_find_path2connect(const func_conn_spec_t *conn_spec, const char *in_alias);
const char *fspec_find_param(const func_param_t *params, const char *p_alias);

const connection_spec_t *fspec_conn_find(const connection_spec_t **conn_spec, const char *alias2find);

int fspec_conn_arr_size(const connection_spec_t **conn_spec);

/**
 * Find function by name in a global registry. This function and registry is implemented by autogen tool fspecgen.py
 * @param spec_name Function specification name
 * @return
 */
const function_handler_t *fspec_find_handler(const char *spec_name);
const function_flow_t *fspec_find_flow(const char *spec_name);

fspec_rv_t function_init(const function_handler_t *fh, const char *inv_name,
                         eswb_topic_descr_t mounting_td, void **dhandle);
fspec_rv_t function_init_inputs(const function_handler_t *fh, void *interface,
                                const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_td);
fspec_rv_t function_init_outputs(const function_handler_t *fh, void *interface,
                                 const func_conn_spec_t *conn_spec, eswb_topic_descr_t mounting_td, const char *func_name);
fspec_rv_t
function_set_param(const function_handler_t *fh, void *interface, const func_param_t *params, int initial_call);
void function_exec(const function_handler_t *fh, void *interface);

const char *fspec_errmsg(fspec_rv_t c);


typedef enum {
    dtp_none = 0,
    dtp_update_prev
} deltatime_policy_t;

fspec_rv_t function_gettime(func_time_t ft, struct timespec *t);
fspec_rv_t function_getdeltatime(func_time_t ft, struct timespec *prev, deltatime_policy_t p, double *dt);

#define CATOM_DEBUG YES
#ifdef CATOM_DEBUG
#include <stdio.h>
#define dbg_msg(txt,...) fprintf(stderr, "%s | " txt "\n", __func__, ##__VA_ARGS__)
#define dbg_msg_ec(fspec_rv,txt,...) fprintf(stderr, "%s | " txt ": %s\n", __func__, ##__VA_ARGS__, fspec_errmsg(fspec_rv))
#else
#define dbg_msg(txt,...)
#define dbg_msg_ec(txt,ec,...)
#endif

#endif //ESWB_PROTO_MODULE_H
