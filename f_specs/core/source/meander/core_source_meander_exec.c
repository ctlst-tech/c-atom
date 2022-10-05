#include "core_source_meander.h"

void core_source_meander_exec(
    core_source_meander_outputs_t *o,
    const core_source_meander_params_t *p,
    core_source_meander_state_t *state
)
{
    state->counter++;
    if(state->counter >= p->semi_period) {
        state->counter = 0;
        if (state->state) {
            state->state = FALSE;
        } else {
            state->state = TRUE;
        }
    }

    o->output = state->state ? 1.0 : 0.0;
}
