#include "atomics_cli_cmd.h"
#include "core_cli_cmd_i32.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"

fspec_rv_t core_cli_cmd_i32_pre_exec_init(const core_cli_cmd_i32_params_t *p, core_cli_cmd_i32_state_t *state)
{
    fspec_rv_t rv;

    state->value = p->default_output;

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, 2);
    topic_proclaiming_tree_t *cmd_fifo_root = usr_topic_set_fifo(cntx, p->alias, 2);
    usr_topic_add_struct_child(cntx, cmd_fifo_root, atomic_cmd_i32_t, cmd_i32, "cmd_i32", tt_int32);

    eswb_rv_t erv = atomic_cli_register_fifo(cntx, cmd_fifo_root, &state->fifo_td);
    if (erv == eswb_e_ok) {
        rv = fspec_rv_ok;
    } else {
        rv = fspec_rv_initerr;
    }

    return rv;
}

void core_cli_cmd_i32_exec(
    core_cli_cmd_i32_outputs_t *o,
    const core_cli_cmd_i32_params_t *p,
    core_cli_cmd_i32_state_t *state,
    core_cli_cmd_i32_outputs_update_flags_t *out_update_cmd
)
{
    atomic_cmd_i32_t cmd;
    eswb_rv_t rv = eswb_fifo_try_pop(state->fifo_td, &cmd);

    if (rv == eswb_e_no_update) {
        // nothing happened
    } else if (rv == eswb_e_ok) {
        o->output = cmd.cmd_i32;
        out_update_cmd->output_updated = 1;
    }
}

