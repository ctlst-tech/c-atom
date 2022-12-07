#include "core_filter_mvng_av.h"

/*
 * FIXME very rough prototype dummy
 *
 * This is fake moving average, which is calculating average once at the moment of selection fill
 */

#define SELECTION_SIZE 50
static core_type_f64_t selection[SELECTION_SIZE][3];
static core_type_f64_t output[3];
static int counter = 0;

fspec_rv_t core_filter_mvng_av_pre_exec_init(core_filter_mvng_av_state_t *state) {

    return fspec_rv_ok;
}

void core_filter_mvng_av_exec(const core_filter_mvng_av_inputs_t *i, core_filter_mvng_av_outputs_t *o, core_filter_mvng_av_state_t *state)
{
    if (counter < SELECTION_SIZE) {
        selection[counter][0] = i->i1;
        selection[counter][1] = i->i2;
        selection[counter][2] = i->i3;
        counter++;

        if (counter == SELECTION_SIZE) {
            output[0] = 0;
            output[1] = 0;
            output[2] = 0;
            for (int j = 0; j < SELECTION_SIZE; j++) {
                output[0] += selection[j][0];
                output[1] += selection[j][1];
                output[2] += selection[j][2];
            }
            output[0] /= SELECTION_SIZE;
            output[1] /= SELECTION_SIZE;
            output[2] /= SELECTION_SIZE;
        }
        o->filed = FALSE;
    } else {
        o->a1 = output[0];
        o->a2 = output[1];
        o->a3 = output[2];
        o->filed = TRUE;
    }
}

