#include "core_filter_delay.h"

void core_filter_delay_exec(
    const core_filter_delay_inputs_t *i,
    core_filter_delay_outputs_t *o,
    const core_filter_delay_params_t *p,
    core_filter_delay_state_t *state
)
{
    state->accumulator.curr_len = state->accumulator.max_len;

    state->accumulator.vector[state->head_index++] = i->input;
    if (state->head_index >= state->accumulator.max_len) {
        state->head_index = 0;
    }

    // output is the next value after just written one
    o->output = state->accumulator.vector[state->head_index];
}
