#include "core_math_gt.h"

void core_math_gt_exec(
    const core_math_gt_inputs_t *i,
    core_math_gt_outputs_t *o
)
{
    o->output = (i->input0) > (i->input1);
}
