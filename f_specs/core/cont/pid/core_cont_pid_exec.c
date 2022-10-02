#include "core_controller_pid.h"

void core_cont_pid_exec(
        const core_controller_pid_inputs_t *i,
        core_controller_pid_outputs_t *o,
        const core_controller_pid_params_t *p,
        core_controller_pid_state_t *state,
        const core_controller_pid_injection_t *injection
)
{
    if (1) {
        double const dt = state->time_from_last_iteration + injection->dt;

        double const error = i->input - i->feedback;

        state->integral_part += 0.5 * p->Ki * (error + state->previous_error) * dt;

        if (state->integral_part > p->integral_max) {
            state->integral_part = p->integral_max;
        } else if (state->integral_part < p->integral_min) {
            state->integral_part = p->integral_min;
        }

        double const P = p->Kp * error;
        double const I = state->integral_part;
        double const D = p->Kd * (error - state->previous_error) / dt;

        o->output = P + I + D;

        if (o->output > p->output_max) {
            o->output = p->output_max;
        } else if (o->output < p->output_min) {
            o->output = p->output_min;
        }

        o->enable = TRUE;

        state->time_from_last_iteration = 0;
        state->previous_error = error;
    } else {
        state->time_from_last_iteration += injection->dt;
        o->enable = FALSE;
    }
}
