#ifndef C_ATOM_SWSYS_H
#define C_ATOM_SWSYS_H

#include "function.h"

#define SWSYS_MAX_TASKS 32
#define SWSYS_MAX_BUSSES 32
#define SWSYS_MAX_SERVICES 8

typedef uint32_t swsys_priority_t;

typedef enum {
    swsys_e_ok = 0,
//    swsys_e_nomem,
//    swsys_e_notopic,
    swsys_e_notask,
    swsys_e_invargs,
    swsys_e_loaderr,
    swsys_e_initerr,
    swsys_e_no_such_service,
    swsys_e_service_fail,
} swsys_rv_t;

typedef enum {
    swsys_clk_none = 0,
    swsys_clk_freerun,
    swsys_clk_timer,
    swsys_clk_inp_upd,
    swsys_clk_ext_call, // ?? e.g. call from IRQ routine
} swsys_task_clk_method_t;

typedef struct swsys_bus_directory {
    const char *path;
    uint32_t eq_channel;
    struct swsys_bus_directory *dirs;
} swsys_bus_directory_t;

typedef struct {
    const char *name;
    swsys_bus_directory_t *dirs;
    uint32_t max_topics;
    uint32_t eq_channel;

    uint32_t evq_size;
    uint32_t evq_buffer_size;
} swsys_bus_t;

typedef enum {
    tsk_flow,
    tsk_fsm,
    tsk_ibr,
    tsk_ebr,
    tsk_arbitrary,
    tsk_unknown
} swsys_task_type_t;


typedef struct {
    const char *name;
    swsys_task_type_t type;
    swsys_priority_t priority;
    const char *config_path;

    function_handler_t  func_handler;
    void *func_call_dhandle;

    func_param_t *params;
    func_conn_spec_t *conn_specs_in;
    func_conn_spec_t *conn_specs_out;

    swsys_task_clk_method_t clk_method;
    int clk_period_ms;
    const char *clk_input_path;

    int exec_once;
} swsys_task_t;

typedef struct {
    const char *name;
    func_param_t *params;
} swsys_service_resource_t;

typedef struct {
    const char *name;
    const char *type;

    swsys_priority_t priority;

    swsys_service_resource_t *resources; // an additional info for service for listing other objects
} swsys_service_t;

typedef struct {
    const char *name;

    swsys_task_t tasks[SWSYS_MAX_TASKS];
    int tasks_num;

    swsys_bus_t busses[SWSYS_MAX_BUSSES];
    int busses_num;

    swsys_service_t services[SWSYS_MAX_SERVICES];
    int services_num;
} swsys_t;


swsys_rv_t swsys_load(const char *path, const char *swsys_root_dir, swsys_t *sys);
swsys_rv_t swsys_top_module_start(swsys_t *swsys);
swsys_rv_t swsys_set_params(swsys_t *sys, const char *task_name, const char *cmd);
const char *swsys_strerror(swsys_rv_t rv);

#endif //C_ATOM_SWSYS_H
