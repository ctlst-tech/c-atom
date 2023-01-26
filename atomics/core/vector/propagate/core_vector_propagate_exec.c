#include "core_vector_propagate.h"

 void core_vector_propagate_exec(
    const core_vector_propagate_inputs_t *i,
    core_vector_propagate_outputs_t *o,
    core_vector_propagate_state_t *state,
    const core_vector_propagate_injection_t *injection
)
{
    if ((i->optional_inputs_flags.enable && i->enable) ||
        (!i->optional_inputs_flags.enable)) {
        if (state->inited) {
            o->v.x = i->v.x + i->derivative.x * injection->dt;
            o->v.y = i->v.y + i->derivative.y * injection->dt;
            o->v.z = i->v.z + i->derivative.z * injection->dt;
        } else {
            o->v.x = i->v0.x;
            o->v.y = i->v0.y;
            o->v.z = i->v0.z;
            state->inited = TRUE;
        }
    }
}

