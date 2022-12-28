#include "core_math_rate_limiter.h"

void core_math_rate_limiter_exec(
    const core_math_rate_limiter_inputs_t *i,
    core_math_rate_limiter_outputs_t *o,
    const core_math_rate_limiter_injection_t *injection
)
{
    double out = o->output;

    if (out < i->input) {
        out += i->rate * injection->dt;
        if (out > i->input) {
            out = i->input;
        }
    } else if (out > i->input) {
        out -= i->rate * injection->dt;
        if (out < i->input) {
            out = i->input;
        }
    } else {
        // Nothing do
    }

    o->output = out;
}
