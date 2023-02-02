#include "core_filter_moving_average.h"

void core_filter_moving_average_exec(
    const core_filter_moving_average_inputs_t *i,
    core_filter_moving_average_outputs_t *o,
    const core_filter_moving_average_params_t *p,
    core_filter_moving_average_state_t *state
) {
    unsigned j;

    state->selection.vector[state->index++] = i->input;
    if (state->index >= p->selection_size) {
        state->index = 0;
    }

    double accumulator = 0;

    for (j = 0; j < p->selection_size; j++) {
        accumulator += state->selection.vector[j];
    }

    o->output = accumulator / p->selection_size;
}
