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
        o->q.w = i->qN.w + -0.5 * ( i->qN.x*i->wx + i->qN.y*i->wy + i->qN.z*i->wz ) * injection->dt;
        o->q.x = i->qN.x +  0.5 * ( i->qN.w*i->wx + i->qN.y*i->wz - i->qN.z*i->wy ) * injection->dt;
        o->q.y = i->qN.y +  0.5 * ( i->qN.w*i->wy + i->qN.z*i->wx - i->qN.x*i->wz ) * injection->dt;
        o->q.z = i->qN.z +  0.5 * ( i->qN.w*i->wz + i->qN.x*i->wy - i->qN.y*i->wx ) * injection->dt;
    }
}
