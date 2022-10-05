#include "core_filter_median.h"

void core_filter_median_exec(
    const core_filter_median_inputs_t *i,
    core_filter_median_outputs_t *o,
    const core_filter_median_params_t *p,
    core_filter_median_state_t *state
)
{
//    if (!state->inited) {
//        state->selection = calloc(p->size, sizeof(i->input));
//        state->inited = TRUE;
//    }
//    core_type_f64_t *sel = state->selection;
//
//    sel[state->counter++] = i->input;
//
//    if (state->counter >= p->size) {
//        state->counter = 0;
//        state->filled_in = TRUE;
//    }
//
//    if (state->filled_in) {
//        for(uint32_t i = 0; i < p->size; i++) {
//
//        }
//    }
}

