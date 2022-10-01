#include <math.h>
#include "core_quat_norm.h"

#define square(s) ((s)*(s))

void core_quat_norm_exec(const core_quat_norm_inputs_t *i, core_quat_norm_outputs_t *o)
{

    core_type_f64_t n = 1.0 / sqrt( square(i->q.w) +  square(i->q.x) +  square(i->q.y) +  square(i->q.z));
    o->q.w = i->q.w * n;
    o->q.x = i->q.x * n;
    o->q.y = i->q.y * n;
    o->q.z = i->q.z * n;
    
}

