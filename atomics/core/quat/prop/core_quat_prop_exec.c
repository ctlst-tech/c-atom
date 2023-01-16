#include "core_quat_prop.h"

void core_quat_prop_exec(
    const core_quat_prop_inputs_t *i,
    core_quat_prop_outputs_t *o,
    core_quat_prop_state_t *state,
    const core_quat_prop_injection_t *injection
)
{
    if (i->optional_inputs_flags.reset) {
        if (i->reset) {
            state->inited = 0;
        }
    }

    if (state->inited == FALSE) {
        o->q = i->q0;
        state->inited = 1;
    } else {
        o->q.w = i->q.w + -0.5 * ( i->q.x * i->omega.x + i->q.y * i->omega.y + i->q.z * i->omega.z ) * injection->dt;
        o->q.x = i->q.x +  0.5 * ( i->q.w * i->omega.x + i->q.y * i->omega.z - i->q.z * i->omega.y ) * injection->dt;
        o->q.y = i->q.y +  0.5 * ( i->q.w * i->omega.y + i->q.z * i->omega.x - i->q.x * i->omega.z ) * injection->dt;
        o->q.z = i->q.z +  0.5 * ( i->q.w * i->omega.z + i->q.x * i->omega.y - i->q.y * i->omega.x ) * injection->dt;
    }
}
