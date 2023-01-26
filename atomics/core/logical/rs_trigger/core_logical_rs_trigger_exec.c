#include "core_logical_rs_trigger.h"

 void core_logical_rs_trigger_exec(
    const core_logical_rs_trigger_inputs_t *i,
    core_logical_rs_trigger_outputs_t *o,
    core_logical_rs_trigger_state_t *state
)
{
    if (i->r && i->s) {
        // ignore
    } else if (i->r) {
        state->state = FALSE;
    } else if (i->s) {
        state->state = TRUE;
    } else {
        // no change
    }

    o->output = state->state;
}

