#include "core_vector_integrate.h"

 void core_vector_integrate_exec(
    const core_vector_integrate_inputs_t *i,
    core_vector_integrate_outputs_t *o,
    core_vector_integrate_state_t *state,
    const core_vector_integrate_injection_t *injection
)
{
    if ((i->optional_inputs_flags.enable && i->enable) || 
        (!i->optional_inputs_flags.enable)) {
        state->integral.x += i->v.x * injection->dt;
        state->integral.y += i->v.y * injection->dt;
        state->integral.z += i->v.z * injection->dt;
    }
    if (i->optional_inputs_flags.reset && i->reset) {
        state->integral.x = 0;
        state->integral.y = 0;
        state->integral.z = 0;
    }
    o->v.x = state->integral.x;
    o->v.y = state->integral.y;
    o->v.z = state->integral.z;
}

