#include "core_cont_pid.h"

void core_cont_pid_exec(
        const core_cont_pid_inputs_t *i,
        core_cont_pid_outputs_t *o,
        const core_cont_pid_params_t *p,
        core_cont_pid_state_t *state,
        const core_cont_pid_injection_t *injection
)
{
    // if enable input is not connected PID is always enabled
    int pid_enabled = (i->optional_in_enable_connected && i->enable) || (!i->optional_in_enable_connected);

    if (pid_enabled) {
        double const dt = state->time_from_last_iteration + injection->dt;
        double const error = i->input - i->feedback;

        if (i->optional_in_preset_connected && !state->activated) {
            state->integral = i->preset;
            state->activated = TRUE;
        } else {
            state->integral += p->Ki *(0.5 * (error + state->previous_error)) * dt;
        }

        if (state->integral > p->integral_max) {
            state->integral = p->integral_max;
        } else if (state->integral < p->integral_min) {
            state->integral = p->integral_min;
        }

        double const P = p->Kp * error;
        double const I = state->integral;
        double const D = p->Kd * (error - state->previous_error) / dt;

        o->output = P + I + D;

        if (o->output > p->output_max) {
            o->output = p->output_max;
        } else if (o->output < p->output_min) {
            o->output = p->output_min;
        }

        o->enabled = TRUE;

        state->time_from_last_iteration = 0;
        state->previous_error = error;
    } else {
        o->enabled = FALSE;
        state->activated = FALSE;
    }

    // state->time_from_last_iteration += injection->dt; --> FIXME will need that after we will be able to "feel" input update event
}
