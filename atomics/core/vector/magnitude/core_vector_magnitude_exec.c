#include <math.h>
#include "core_vector_magnitude.h"

void core_vector_magnitude_exec(const core_vector_magnitude_inputs_t *i, core_vector_magnitude_outputs_t *o)
{
    o->output = sqrt(i->v.x * i->v.x + i->v.y * i->v.y);
}
