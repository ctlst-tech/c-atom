#include <float.h>  // For DBL_MAX, DBL_EPSILON
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "atomics_cli_cmd.h"
#include "core_calib_v3f64.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"

// Calibration FSM Stages (Revised for generic axis processing)
#define CALIB_STAGE_IDLE 0
#define CALIB_STAGE_START_AXIS_SEQUENCE 5  // Initializes for the first axis (X)
#define CALIB_STAGE_PROMPT_EXTREME 10      // Prompts user for current axis/extreme
#define CALIB_STAGE_SAMPLING_EXTREME 11    // Collects and averages samples for the current extreme
#define CALIB_STAGE_CALCULATE_SAVE 40      // All axes sampled, calculate K/B and save
#define CALIB_STAGE_DONE_APPLYING 50       // Calibration successful, applying new coefficients
#define CALIB_STAGE_ERROR 60               // Error occurred

// User FSM Commands (from stage_cmd_fifo_td)
#define CALIB_CMD_NONE 0
#define CALIB_CMD_NEXT_STAGE 1
#define CALIB_CMD_RESTART_CALIBRATION 2
#define CALIB_CMD_CANCEL_CALIBRATION 3

// Helper to get axis name as string
static const char* get_axis_name(uint8_t axis_idx) {
    if (axis_idx == 0) return "X";
    if (axis_idx == 1) return "Y";
    if (axis_idx == 2) return "Z";
    return "UNKNOWN";
}

// Helper to get the input value for the current sampling axis
static double get_input_for_axis(const core_calib_v3f64_inputs_t *i, uint8_t axis_idx) {
    if (axis_idx == 0) return i->input.x;
    if (axis_idx == 1) return i->input.y;
    if (axis_idx == 2) return i->input.z;
    return 0.0; // Should not happen
}

// Helper to store the determined min/max averaged values for an axis
static void store_axis_min_max_avg(core_calib_v3f64_state_t *state, uint8_t axis_idx, double avg1, double avg2) {
    double min_val = (avg1 < avg2) ? avg1 : avg2;
    double max_val = (avg1 > avg2) ? avg1 : avg2;

    if (axis_idx == 0) {
        state->min_x_raw_avg = min_val;
        state->max_x_raw_avg = max_val;
    } else if (axis_idx == 1) {
        state->min_y_raw_avg = min_val;
        state->max_y_raw_avg = max_val;
    } else if (axis_idx == 2) {
        state->min_z_raw_avg = min_val;
        state->max_z_raw_avg = max_val;
    }
}


// Helper to reset averaged min/max raw values
static void reset_averaged_min_max_data(core_calib_v3f64_state_t *state) {
    state->min_x_raw_avg = DBL_MAX; state->max_x_raw_avg = -DBL_MAX;
    state->min_y_raw_avg = DBL_MAX; state->max_y_raw_avg = -DBL_MAX;
    state->min_z_raw_avg = DBL_MAX; state->max_z_raw_avg = -DBL_MAX;
    state->num_samples_in_accumulator = 0;
    state->current_sampling_axis_idx = 0; // Start with X-axis
    state->is_positive_extreme_next = true; // Start with positive extreme
    state->current_axis_avg_extreme1 = 0.0;
}

// Load calibration data from file (remains similar)
static bool load_calibration_data(const char *path, core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) return false;
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("CALIB: No calibration file '%s'. Using initial/current parameters.\n", path);
        return false;
    }
    if (fscanf(f, "%lf %lf %lf\n%lf %lf %lf",
               &state->current_b.x, &state->current_b.y, &state->current_b.z,
               &state->current_k.x, &state->current_k.y, &state->current_k.z) == 6) {
        printf("CALIB: Loaded K={%.4f,%.4f,%.4f}, B={%.4f,%.4f,%.4f} from '%s'\n",
               state->current_k.x, state->current_k.y, state->current_k.z,
               state->current_b.x, state->current_b.y, state->current_b.z, path);
        fclose(f);
        return true;
    }
    printf("CALIB_ERROR: Reading file '%s'. Using initial/current parameters.\n", path);
    fclose(f);
    return false;
}

// Save calibration data to file (remains similar)
static bool save_calibration_data(const char *path, const core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) return false;
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "CALIB_ERROR: Opening file '%s' for writing: ", path); perror(NULL);
        return false;
    }
    fprintf(f, "%.10g %.10g %.10g\n", state->current_b.x, state->current_b.y, state->current_b.z);
    fprintf(f, "%.10g %.10g %.10g\n", state->current_k.x, state->current_k.y, state->current_k.z);
    fclose(f);
    printf("CALIB: Saved K={%.4f,%.4f,%.4f}, B={%.4f,%.4f,%.4f} to '%s'\n",
           state->current_k.x, state->current_k.y, state->current_k.z,
           state->current_b.x, state->current_b.y, state->current_b.z, path);
    return true;
}


// Finite State Machine Handler
static void handle_calibration_fsm(
    const core_calib_v3f64_inputs_t *i,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state,
    int32_t user_fsm_cmd) {

    char cli_cmd_topic_name[128];
    snprintf(cli_cmd_topic_name, sizeof(cli_cmd_topic_name), "%s/cmd", p->cli_base_alias);

    // Handle global commands
    if (user_fsm_cmd == CALIB_CMD_CANCEL_CALIBRATION) {
        printf("CALIB: CANCEL received. Calibration stopped.\n");
        state->calibration_active = false;
        state->calib_stage = CALIB_STAGE_IDLE;
        state->current_k.x = p->initial_k1; state->current_k.y = p->initial_k2; state->current_k.z = p->initial_k3;
        state->current_b.x = p->initial_b1; state->current_b.y = p->initial_b2; state->current_b.z = p->initial_b3;
        load_calibration_data(p->settings_path, state); // Try to reload saved, else initial are kept
        printf("CALIB: Reverted to initial/saved coefficients.\n");
        return;
    }
    if (user_fsm_cmd == CALIB_CMD_RESTART_CALIBRATION) {
        printf("CALIB: RESTART received. Restarting calibration.\n");
        state->calibration_active = true;
        reset_averaged_min_max_data(state);
        state->calib_stage = CALIB_STAGE_START_AXIS_SEQUENCE;
        // Fall through to START_AXIS_SEQUENCE logic in this call
    }

    if (!state->calibration_active && state->calib_stage != CALIB_STAGE_IDLE) {
         if (state->calib_stage > CALIB_STAGE_IDLE && state->calib_stage < CALIB_STAGE_DONE_APPLYING) {
             // printf("CALIB_FSM_WARN: Calibration became inactive mid-process. Reverting to IDLE.\n");
         }
        state->calib_stage = CALIB_STAGE_IDLE;
        return;
    }

    switch (state->calib_stage) {
        case CALIB_STAGE_IDLE:
            if (state->calibration_active) {
                printf("CALIB: Calibration enabled. To control, send commands to CLI topic '%s'.\n", cli_cmd_topic_name);
                printf("CALIB_INFO: Commands: NEXT_STAGE (%d), RESTART (%d), CANCEL (%d).\n",
                       CALIB_CMD_NEXT_STAGE, CALIB_CMD_RESTART_CALIBRATION, CALIB_CMD_CANCEL_CALIBRATION);
                state->calib_stage = CALIB_STAGE_START_AXIS_SEQUENCE;
                reset_averaged_min_max_data(state); // Prepare for first axis
            }
            break;

        case CALIB_STAGE_START_AXIS_SEQUENCE:
            printf("CALIB: Initializing for %s-axis calibration.\n", get_axis_name(state->current_sampling_axis_idx));
            state->is_positive_extreme_next = true;
            state->num_samples_in_accumulator = 0;
            state->calib_stage = CALIB_STAGE_PROMPT_EXTREME;
            // Fall through to print prompt in this cycle

        case CALIB_STAGE_PROMPT_EXTREME:
            {
                const char* axis_name = get_axis_name(state->current_sampling_axis_idx);
                const char* extreme_direction = state->is_positive_extreme_next ? "positive" : "negative";
                const char* example_orientation = "";
                if (strcmp(axis_name, "X") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., X-axis pointing UP" : "e.g., X-axis pointing DOWN";
                else if (strcmp(axis_name, "Y") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., Y-axis pointing UP" : "e.g., Y-axis pointing DOWN";
                else if (strcmp(axis_name, "Z") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., Z-axis pointing UP" : "e.g., Z-axis pointing DOWN";

                printf("CALIB: Position sensor for %s %s-axis extreme (%s) and hold steady.\n",
                       extreme_direction, axis_name, example_orientation);
                printf("CALIB: Send NEXT_STAGE (%d) to '%s' to start sampling for this position.\n", CALIB_CMD_NEXT_STAGE, cli_cmd_topic_name);

                if (user_fsm_cmd == CALIB_CMD_NEXT_STAGE) {
                    printf("CALIB: Starting %s %s-axis sampling for %u samples.\n",
                           extreme_direction, axis_name, p->selection_size);
                    state->num_samples_in_accumulator = 0; // Reset for this specific sampling run
                    // Ensure sample_accumulator.vector is valid if using dynamic allocation (fspec should handle)
                    if (state->sample_accumulator.vector == NULL && p->selection_size > 0) {
                         fprintf(stderr, "CALIB_ERROR: Sample accumulator buffer is NULL!\n");
                         state->calib_stage = CALIB_STAGE_ERROR;
                         break;
                    }
                    state->calib_stage = CALIB_STAGE_SAMPLING_EXTREME;
                }
            }
            break;

        case CALIB_STAGE_SAMPLING_EXTREME:
            if (state->sample_accumulator.vector == NULL && p->selection_size > 0) {
                 fprintf(stderr, "CALIB_ERROR: Sample accumulator buffer is NULL during sampling!\n");
                 state->calib_stage = CALIB_STAGE_ERROR;
                 break;
            }
            if (state->num_samples_in_accumulator < p->selection_size && state->num_samples_in_accumulator < state->sample_accumulator.max_len) {
                state->sample_accumulator.vector[state->num_samples_in_accumulator] = get_input_for_axis(i, state->current_sampling_axis_idx);
                state->num_samples_in_accumulator++;
            }

            if (state->num_samples_in_accumulator >= p->selection_size) {
                double sum = 0;
                for (uint16_t k = 0; k < p->selection_size; ++k) {
                    sum += state->sample_accumulator.vector[k];
                }
                double current_average = (p->selection_size > 0) ? (sum / p->selection_size) : 0.0;

                const char* axis_name = get_axis_name(state->current_sampling_axis_idx);
                printf("CALIB: %s-axis, %s extreme sampling complete. Averaged value: %.4f\n",
                       axis_name, state->is_positive_extreme_next ? "positive" : "negative", current_average);

                if (state->is_positive_extreme_next) {
                    state->current_axis_avg_extreme1 = current_average;
                    state->is_positive_extreme_next = false; // Move to negative extreme for current axis
                    state->calib_stage = CALIB_STAGE_PROMPT_EXTREME; // Prompt for negative side
                } else { // Negative extreme done for current_sampling_axis_idx
                    store_axis_min_max_avg(state, state->current_sampling_axis_idx, state->current_axis_avg_extreme1, current_average);
                    printf("CALIB: %s-axis min/max averages determined: Min=%.4f, Max=%.4f.\n", axis_name,
                        (state->current_axis_avg_extreme1 < current_average ? state->current_axis_avg_extreme1 : current_average),
                        (state->current_axis_avg_extreme1 > current_average ? state->current_axis_avg_extreme1 : current_average)
                    );

                    // Check if min/max are too close
                    double range_check;
                    if(state->current_sampling_axis_idx == 0) range_check = fabs(state->max_x_raw_avg - state->min_x_raw_avg);
                    else if(state->current_sampling_axis_idx == 1) range_check = fabs(state->max_y_raw_avg - state->min_y_raw_avg);
                    else range_check = fabs(state->max_z_raw_avg - state->min_z_raw_avg);

                    if (range_check < 1e-6 * fabs(p->target_calibrated_magnitude_per_axis)) {
                         printf("CALIB_ERROR: %s-Axis min/max averages are too close (%.4f, %.4f). Insufficient movement or sensor issue.\n",
                               axis_name,
                               (state->current_axis_avg_extreme1 < current_average ? state->current_axis_avg_extreme1 : current_average),
                               (state->current_axis_avg_extreme1 > current_average ? state->current_axis_avg_extreme1 : current_average)
                               );
                         state->calib_stage = CALIB_STAGE_ERROR;
                         break;
                    }

                    state->current_sampling_axis_idx++; // Move to next axis
                    if (state->current_sampling_axis_idx < 3) { // Still axes to calibrate (0,1,2 for X,Y,Z)
                        state->is_positive_extreme_next = true; // Start with positive for next axis
                        printf("CALIB: Advancing to %s-axis calibration.\n", get_axis_name(state->current_sampling_axis_idx));
                        state->calib_stage = CALIB_STAGE_PROMPT_EXTREME;
                    } else { // All axes done
                        printf("CALIB: All axes sampled. Proceeding to calculate coefficients.\n");
                        state->calib_stage = CALIB_STAGE_CALCULATE_SAVE;
                    }
                }
                state->num_samples_in_accumulator = 0; // Reset for next sampling run
            }
            break;

        case CALIB_STAGE_CALCULATE_SAVE:
            printf("CALIB: Calculating and saving calibration coefficients using averaged min/max.\n");
            state->current_b.x = (state->max_x_raw_avg + state->min_x_raw_avg) / 2.0;
            state->current_b.y = (state->max_y_raw_avg + state->min_y_raw_avg) / 2.0;
            state->current_b.z = (state->max_z_raw_avg + state->min_z_raw_avg) / 2.0;

            double range_x = state->max_x_raw_avg - state->min_x_raw_avg;
            double range_y = state->max_y_raw_avg - state->min_y_raw_avg;
            double range_z = state->max_z_raw_avg - state->min_z_raw_avg;

            state->current_k.x = (fabs(range_x) > DBL_EPSILON) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_x : 1.0;
            state->current_k.y = (fabs(range_y) > DBL_EPSILON) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_y : 1.0;
            state->current_k.z = (fabs(range_z) > DBL_EPSILON) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_z : 1.0;

            printf("CALIB: New B: {%.4f,%.4f,%.4f}, K: {%.4f,%.4f,%.4f}\n",
                   state->current_b.x, state->current_b.y, state->current_b.z,
                   state->current_k.x, state->current_k.y, state->current_k.z);

            save_calibration_data(p->settings_path, state);
            state->calib_stage = CALIB_STAGE_DONE_APPLYING;
            printf("CALIB: Calibration complete. Applying new coefficients.\n");
            state->calibration_active = false; // Calibration process itself is done
            break;

        case CALIB_STAGE_DONE_APPLYING:
             if (state->calibration_active) { // User re-enables
                printf("CALIB: Re-initiating calibration from DONE_APPLYING due to enable signal.\n");
                reset_averaged_min_max_data(state);
                state->calib_stage = CALIB_STAGE_START_AXIS_SEQUENCE;
            }
            break;

        case CALIB_STAGE_ERROR:
            printf("CALIB_ERROR: FSM in error state. Calibration halted.\n");
            printf("CALIB_ERROR: Send RESTART (%d) or CANCEL (%d) to '%s' to proceed.\n",
                   CALIB_CMD_RESTART_CALIBRATION, CALIB_CMD_CANCEL_CALIBRATION, cli_cmd_topic_name);
            state->calibration_active = false; // Stop further FSM processing unless command received
            break;

        default:
            fprintf(stderr, "CALIB_ERROR: Unknown stage: %d. Resetting to IDLE.\n", state->calib_stage);
            state->calib_stage = CALIB_STAGE_IDLE;
            state->calibration_active = false;
            break;
    }
}


static void process_calibration_logic(
    const core_calib_v3f64_inputs_t *i,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state) {

    // 1. Handle calibration activation timeout
    if (p->calibration_start_timeout_iterations > 0 && !state->calibration_disabled && !state->calibration_active) {
        state->init_iterations_counter++;
        if (state->init_iterations_counter > p->calibration_start_timeout_iterations) {
            state->calibration_disabled = true;
            printf("CALIB: Activation timeout reached. Calibration permanently disabled.\n");
        }
    }
     // If permanently disabled, ensure calibration_active is false and exit.
    if (state->calibration_disabled) {
        if(state->calibration_active) {
            state->calibration_active = false; // Force disable
            state->calib_stage = CALIB_STAGE_IDLE;
        }
        return;
    }


    // 2. Check for CLI commands
    atomic_cmd_bool_t enable_cmd;
    bool prev_calibration_active = state->calibration_active;
    if (eswb_fifo_try_pop(state->enable_cmd_fifo_td, &enable_cmd) == eswb_e_ok) {
        if (state->calibration_disabled && enable_cmd.cmd_bool) {
             printf("CALIB: '%s/enable' CLI: Enable ignored, calibration permanently disabled.\n", p->cli_base_alias);
        } else if (state->calibration_active != enable_cmd.cmd_bool) {
            state->calibration_active = enable_cmd.cmd_bool;
            printf("CALIB: '%s/enable' CLI: Calibration active set to %s.\n", p->cli_base_alias, state->calibration_active ? "TRUE" : "FALSE");

            if (state->calibration_active) {
                 if (state->calib_stage == CALIB_STAGE_IDLE || state->calib_stage == CALIB_STAGE_DONE_APPLYING || state->calib_stage == CALIB_STAGE_ERROR) {
                    state->calib_stage = CALIB_STAGE_IDLE; // FSM will transition to START_AXIS_SEQUENCE
                    reset_averaged_min_max_data(state);
                    printf("CALIB: Calibration (re)activated. Will proceed from initial stage.\n");
                }
            } else { // Disabled by user
                if (prev_calibration_active) {
                     printf("CALIB: Calibration explicitly disabled. Current K/B params used.\n");
                      if (state->calib_stage > CALIB_STAGE_IDLE && state->calib_stage < CALIB_STAGE_DONE_APPLYING) {
                         printf("CALIB: Calibration was in progress. Halted. Stage reset to IDLE.\n");
                     }
                     state->calib_stage = CALIB_STAGE_IDLE;
                }
            }
        }
    }

    int32_t user_fsm_cmd = CALIB_CMD_NONE;
    atomic_cmd_i32_t stage_fsm_payload;
    if (state->calibration_active || state->calib_stage == CALIB_STAGE_ERROR) { // Allow commands if active or in error
        if (eswb_fifo_try_pop(state->stage_cmd_fifo_td, &stage_fsm_payload) == eswb_e_ok) {
            user_fsm_cmd = stage_fsm_payload.cmd_i32;
            printf("CALIB: '%s/cmd' CLI: Received FSM command: %d\n", p->cli_base_alias, user_fsm_cmd);
        }
    }

    // 3. Execute FSM
    if (state->calibration_active || user_fsm_cmd != CALIB_CMD_NONE || (state->calibration_active && !prev_calibration_active)) {
        state->fsm_iteration_counter++;
        uint16_t decimation = (p->decimation_factor == 0) ? 1 : p->decimation_factor;

        if ((state->calibration_active && state->fsm_iteration_counter >= decimation) || user_fsm_cmd != CALIB_CMD_NONE || (state->calibration_active && !prev_calibration_active)) {
            state->fsm_iteration_counter = 0;
            handle_calibration_fsm(i, p, state, user_fsm_cmd);
        }
    }
}


fspec_rv_t core_calib_v3f64_pre_exec_init(const core_calib_v3f64_params_t *p, core_calib_v3f64_state_t *state) {
    state->calib_stage = CALIB_STAGE_IDLE;
    state->calibration_active = false;
    state->fsm_iteration_counter = 0;

    state->current_k.x = p->initial_k1; state->current_k.y = p->initial_k2; state->current_k.z = p->initial_k3;
    state->current_b.x = p->initial_b1; state->current_b.y = p->initial_b2; state->current_b.z = p->initial_b3;

    reset_averaged_min_max_data(state); // Initializes new sampling state vars too

    state->init_iterations_counter = 0;
    state->calibration_disabled = false;

    // sample_accumulator.vector is expected to be allocated by the fspec framework
    // based on VectorTypeRef and selection_size parameter.
    // We need to check if p->selection_size is valid for state->sample_accumulator.max_len
    // For now, assume state->sample_accumulator.max_len is correctly set by fspec.
    // If selection_size is 0, sample_accumulator.vector might be NULL, handle in FSM.
    if (p->selection_size == 0) {
        printf("CALIB_INIT_WARN: selection_size is 0. Averaging will not occur.\n");
        // FSM logic should handle p->selection_size == 0 gracefully (e.g. take one sample or error)
    }
    // state->sample_accumulator.curr_len is not used by fspec type directly,
    // state->num_samples_in_accumulator is our application counter.

    if (load_calibration_data(p->settings_path, state)) {
        state->calib_stage = CALIB_STAGE_DONE_APPLYING;
        printf("CALIB_INIT: Loaded calibration from '%s'. Stage: DONE_APPLYING.\n", p->settings_path);
    } else {
        printf("CALIB_INIT: Using initial K,B. Stage: IDLE.\n");
    }

    fspec_rv_t rv = fspec_rv_ok;
    eswb_rv_t erv;
    char topic_name_buf[ESWB_TOPIC_NAME_MAX_LEN + 1];

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_enable, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s_enable", p->cli_base_alias);
    topic_proclaiming_tree_t *enable_cmd_root = usr_topic_set_fifo(cntx_enable, topic_name_buf, 2);
    usr_topic_add_struct_child(cntx_enable, enable_cmd_root, atomic_cmd_bool_t, cmd_bool, "cmd_bool", tt_bool);
    erv = atomic_cli_register_fifo(topic_name_buf, cntx_enable, enable_cmd_root, &state->enable_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        fprintf(stderr, "CALIB_INIT_ERROR: Registering enable CLI for '%s' (%s)\n", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_stage, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s_cmd", p->cli_base_alias);
    topic_proclaiming_tree_t *stage_cmd_root = usr_topic_set_fifo(cntx_stage, topic_name_buf, 2);
    usr_topic_add_struct_child(cntx_stage, stage_cmd_root, atomic_cmd_i32_t, cmd_i32, "cmd_i32", tt_int32);
    erv = atomic_cli_register_fifo(topic_name_buf, cntx_stage, stage_cmd_root, &state->stage_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        fprintf(stderr, "CALIB_INIT_ERROR: Registering stage_cmd CLI for '%s' (%s)\n", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    return rv;
}

void core_calib_v3f64_exec(
    const core_calib_v3f64_inputs_t *i,
    core_calib_v3f64_outputs_t *o,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state
) {
    // 1. Process calibration logic (CLI commands, FSM state transitions, data collection)
    // The check for state->calibration_permanently_disabled is now inside process_calibration_logic
    if (!state->calibration_disabled) {
        process_calibration_logic(i, p, state);
    }

    // 2. Always Apply Current Calibration Coefficients to the output
    o->v.x = (i->input.x - state->current_b.x) * state->current_k.x;
    o->v.y = (i->input.y - state->current_b.y) * state->current_k.y;
    o->v.z = (i->input.z - state->current_b.z) * state->current_k.z;
}
