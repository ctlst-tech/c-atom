#include "core_math_polynomial.h"

fspec_rv_t core_math_polynomial_pre_exec_init(
    const core_math_polynomial_params_t *p,
    core_math_polynomial_state_t *state) {
    char *coeffs = strdup(p->coeffs);
    size_t len = strlen(coeffs);
    char *token = coeffs;

    while ((token = strsep(&coeffs, ",")) != NULL) {
        double tmp;
        int number = sscanf(token, "%lf", &tmp);
        if (number == 1) {
            if (state->coeffs.curr_len + 8 > state->coeffs.max_len) {
                return fspec_rv_inval_param;
            }
            state->coeffs.vector[state->coeffs.curr_len/sizeof(double)] = tmp;
            state->coeffs.curr_len += sizeof(double);
        } else if (number == EOF) {
            break;
        } else {
            return fspec_rv_inval_param;
        }
    }
    return fspec_rv_ok;
}

void core_math_polynomial_exec(const core_math_polynomial_inputs_t *i,
                               core_math_polynomial_outputs_t *o,
                               const core_math_polynomial_params_t *p,
                               core_math_polynomial_state_t *state) {
    double out = state->coeffs.vector[0];
    double input_degree = 1;
    for (size_t index = 1; index < state->coeffs.curr_len / sizeof(double); index++) {
        input_degree *= i->input;
        out += input_degree * state->coeffs.vector[index]; 
    }
    o->output = out;
}
