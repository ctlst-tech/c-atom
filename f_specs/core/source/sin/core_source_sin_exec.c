#include "core_source_sin.h"
#include <math.h>

void core_source_sin_exec(
    core_source_sin_outputs_t *o,
    const core_source_sin_params_t *p,
    core_source_sin_state_t *state,
    const core_source_sin_injection_t *injection
)
{
    state->time += injection->dt;
    o->out = sin(p->phase + p->omega * state->time);
}
