#include "core_quat_prop.h"

void core_quat_prop_exec(
    const core_quat_prop_inputs_t *i,
    core_quat_prop_outputs_t *o,
    core_quat_prop_state_t *state,
    const core_quat_prop_injection_t *injection
)
{
    if (!state->inited) {
        o->q = i->q0;
        state->inited = 1;
    } else {
        o->q.w = i->q.w + -0.5 * ( i->q.x*i->wx + i->q.y*i->wy + i->q.z*i->wz ) * injection->dt;
        o->q.x = i->q.x +  0.5 * ( i->q.w*i->wx + i->q.y*i->wz - i->q.z*i->wy ) * injection->dt;
        o->q.y = i->q.y +  0.5 * ( i->q.w*i->wy + i->q.z*i->wx - i->q.x*i->wz ) * injection->dt;
        o->q.z = i->q.z +  0.5 * ( i->q.w*i->wz + i->q.x*i->wy - i->q.y*i->wx ) * injection->dt;
    }
}
