#include "core_source_meander.h"

void core_source_meander_exec(
    core_source_meander_outputs_t *o,
    const core_source_meander_params_t *p,
    core_source_meander_state_t *state,
    const core_source_meander_injection_t *injection
)
{
    // TODO: calculate absolute time
    if(state->time < (p->period * p->duty)) {
        o->out = 0;
    } else {
        o->out = 1;
    }
    state->time++;
    if(state->time > p->period) {
        o->out = 0;
        state->time = 0;
    }
}

