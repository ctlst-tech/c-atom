#include "core_math_lerp.h"

void core_math_lerp_exec(
    const core_math_lerp_inputs_t *i,
    core_math_lerp_outputs_t *o
)
{
    if (i->start < i->end) {
        if (i->factor <= i->start) {
            o->output = i->start;
        } else if (i->factor >= i->end) {
            o->output = i->end;
        } else {
            o->output = i->start + (i->end - i->start) * i->factor;
        }
    } else {
        if (i->factor <= i->end) {
            o->output = i->end;
        } else if (i->factor >= i->start) {
            o->output = i->start;
        } else {
            o->output = i->start + (i->end - i->start) * i->factor;
        }
    }
}
