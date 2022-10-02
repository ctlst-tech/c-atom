#include <math.h>
#include "core_math_atan2.h"

void core_math_atan2_exec(const core_math_atan2_inputs_t *i, core_math_atan2_outputs_t *o)
{
    o->output = atan2(i->input0, i->input1);
}
