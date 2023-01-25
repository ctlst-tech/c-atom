#include "core_vector_sub.h"

 void core_vector_sub_exec(const core_vector_sub_inputs_t *i, core_vector_sub_outputs_t *o)
{
    o->v.x = i->v1.x - i->v2.x;
    o->v.y = i->v1.y - i->v2.y;
    o->v.z = i->v1.z - i->v2.z;
}

