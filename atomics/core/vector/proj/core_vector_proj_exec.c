#include "core_vector_proj.h"
#include <math.h>

 void core_vector_proj_exec(const core_vector_proj_inputs_t *i, core_vector_proj_outputs_t *o)
{
    o->a = atan2(i->v.x, i->v.y);
    o->m = sqrt(i->v.x * i->v.x + i->v.y * i->v.y);
}

