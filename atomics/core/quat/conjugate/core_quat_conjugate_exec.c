#include "core_quat_conjugate.h"

void core_quat_conjugate_exec(const core_quat_conjugate_inputs_t *i, core_quat_conjugate_outputs_t *o)
{
    o->q.w =  i->q.w;
    o->q.x = -i->q.x;
    o->q.y = -i->q.y;
    o->q.z = -i->q.z;
}
