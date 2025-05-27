#include <float.h>  // For DBL_MAX, DBL_MIN
#include <math.h>   // For sqrt, fabs
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "atomics_cli_cmd.h"  // For atomic_cmd_bool_t, atomic_cmd_i32_t
#include "core_calib_v3f64.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"

// Calibration FSM Stages
#define CALIB_STAGE_IDLE 0
#define CALIB_STAGE_WAITING_FOR_ENABLE 1 // Waiting for calibration_active_request to be true
#define CALIB_STAGE_X_AXIS_INIT 10
#define CALIB_STAGE_X_AXIS_PROMPT 11
#define CALIB_STAGE_X_AXIS_SAMPLING 12
#define CALIB_STAGE_Y_AXIS_INIT 20
#define CALIB_STAGE_Y_AXIS_PROMPT 21
#define CALIB_STAGE_Y_AXIS_SAMPLING 22
#define CALIB_STAGE_Z_AXIS_INIT 30
#define CALIB_STAGE_Z_AXIS_PROMPT 31
#define CALIB_STAGE_Z_AXIS_SAMPLING 32
#define CALIB_STAGE_CALCULATE_SAVE 40
#define CALIB_STAGE_DONE_APPLYING 50 // Calibration successful, applying new coefficients
#define CALIB_STAGE_ERROR 60         // Some error occurred

// User FSM Commands (from stage_cmd_fifo_td)
#define CALIB_CMD_NONE 0
#define CALIB_CMD_NEXT_STAGE 1     // User confirms current step, proceed
#define CALIB_CMD_RESTART_CALIBRATION 2 // Restart entire calibration process
#define CALIB_CMD_CANCEL_CALIBRATION 3  // Abort calibration, revert to loaded/initial params

// Helper to initialize min/max values
static void reset_min_max(core_calib_v3f64_state_t *state) {
    state->min_x_raw = DBL_MAX;
    state->max_x_raw = -DBL_MAX;
    state->min_y_raw = DBL_MAX;
    state->max_y_raw = -DBL_MAX;
    state->min_z_raw = DBL_MAX;
    state->max_z_raw = -DBL_MAX;
    state->samples_collected_current_axis = 0;
}

// Load calibration data from file
static bool load_calibration_data(const char *path,
                                  core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) {
        return false;  // No path provided
    }
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf(
            "CALIB: No calibration file found at %s. Using initial "
            "parameters.\n",
            path);
        return false;
    }
    char buffer[256];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes <= 0) {
        printf(
            "CALIB: Error reading calibration file %s. Using initial "
            "parameters.\n",
            path);
        return false;
    }
    buffer[bytes] = '\0';

    // Expecting: bX bY bZ on first line, kX kY kZ on second line
    if (sscanf(buffer, "%lf %lf %lf\n%lf %lf %lf", &state->current_b.x,
               &state->current_b.y, &state->current_b.z, &state->current_k.x,
               &state->current_k.y, &state->current_k.z) == 6) {
        printf(
            "CALIB: Loaded K={%.4f, %.4f, %.4f}, B={%.4f, %.4f, %.4f} from "
            "%s\n",
            state->current_k.x, state->current_k.y, state->current_k.z,
            state->current_b.x, state->current_b.y, state->current_b.z, path);
        return true;
    }
    printf(
        "CALIB: Error parsing calibration file %s. Using initial parameters.\n",
        path);
    return false;
}

// Save calibration data to file
static bool save_calibration_data(const char *path, const core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) {
        return false; // No path provided
    }
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("CALIB: Error opening calibration file for writing");
        return false;
    }
    fprintf(f, "%.10g %.10g %.10g\n", state->current_b.x, state->current_b.y, state->current_b.z);
    fprintf(f, "%.10g %.10g %.10g\n", state->current_k.x, state->current_k.y, state->current_k.z);
    fclose(f);
    printf("CALIB: Saved K={%.4f, %.4f, %.4f}, B={%.4f, %.4f, %.4f} to %s\n",
           state->current_k.x, state->current_k.y, state->current_k.z,
           state->current_b.x, state->current_b.y, state->current_b.z, path);
    return true;
}


static void handle_calibration_fsm(
    const core_calib_v3f64_inputs_t *i,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state) {

    // Handle global commands first
    if (state->user_fsm_command == CALIB_CMD_CANCEL_CALIBRATION) {
        printf("CALIB: Calibration cancelled by user command.\n");
        state->calibration_active_request = false; // Stop calibration process
        state->calib_stage = CALIB_STAGE_IDLE;
        // Reload initial/saved parameters
        state->current_k.x = p->initial_k1; state->current_k.y = p->initial_k2; state->current_k.z = p->initial_k3;
        state->current_b.x = p->initial_b1; state->current_b.y = p->initial_b2; state->current_b.z = p->initial_b3;
        load_calibration_data(p->settings_path, state); // Attempt to load from file over initial
        return; // Command processed
    }

    if (state->user_fsm_command == CALIB_CMD_RESTART_CALIBRATION) {
        printf("CALIB: Restarting calibration by user command.\n");
        state->calibration_active_request = true; // Ensure it's active if restarted
        state->calib_stage = CALIB_STAGE_X_AXIS_INIT; // Go to the very beginning
        reset_min_max(state);
        return; // Command processed
    }

    // If calibration is not active, FSM stays in IDLE or WAITING
    if (!state->calibration_active_request) {
        if (state->calib_stage != CALIB_STAGE_IDLE) {
             printf("CALIB: Calibration not active, returning to IDLE.\n");
        }
        state->calib_stage = CALIB_STAGE_IDLE;
        return;
    }

    // --- Calibration Active FSM ---
    switch (state->calib_stage) {
        case CALIB_STAGE_IDLE:
            if (state->calibration_active_request) {
                printf("CALIB: Calibration enabled. Moving to X-Axis Init.\n");
                state->calib_stage = CALIB_STAGE_X_AXIS_INIT;
            }
            break;

        case CALIB_STAGE_X_AXIS_INIT:
            printf("CALIB: X-Axis Init. Send NEXT_STAGE command when ready to sample X-axis min/max.\n");
            printf("CALIB: Rotate sensor slowly to capture full range for X-axis.\n");
            reset_min_max(state); // Reset for current axis
            state->calib_stage = CALIB_STAGE_X_AXIS_PROMPT;
            break;

        case CALIB_STAGE_X_AXIS_PROMPT:
            if (state->user_fsm_command == CALIB_CMD_NEXT_STAGE) {
                printf("CALIB: Starting X-axis sampling for %u samples.\n", p->selection_size);
                state->samples_collected_current_axis = 0;
                state->calib_stage = CALIB_STAGE_X_AXIS_SAMPLING;
            }
            break;

        case CALIB_STAGE_X_AXIS_SAMPLING:
            if (i->input.x < state->min_x_raw) state->min_x_raw = i->input.x;
            if (i->input.x > state->max_x_raw) state->max_x_raw = i->input.x;
            state->samples_collected_current_axis++;
            if (state->samples_collected_current_axis >= p->selection_size) {
                printf("CALIB: X-Axis sampling complete. Min: %.4f, Max: %.4f\n", state->min_x_raw, state->max_x_raw);
                if (fabs(state->max_x_raw - state->min_x_raw) < 1e-6) { // Check for valid range
                    printf("CALIB_ERROR: X-Axis min/max are too close. Restarting calibration.\n");
                    state->calib_stage = CALIB_STAGE_ERROR;
                    break;
                }
                state->calib_stage = CALIB_STAGE_Y_AXIS_INIT;
            }
            break;

        case CALIB_STAGE_Y_AXIS_INIT:
            printf("CALIB: Y-Axis Init. Send NEXT_STAGE command when ready to sample Y-axis min/max.\n");
            printf("CALIB: Rotate sensor slowly to capture full range for Y-axis.\n");
            // min/max for Y will be reset implicitly if we came from X's completion correctly.
            // If jumping here via debug, ensure reset_min_max was called or do it here.
            state->samples_collected_current_axis = 0; // Reset sample count for this axis
            state->calib_stage = CALIB_STAGE_Y_AXIS_PROMPT;
            break;

        case CALIB_STAGE_Y_AXIS_PROMPT:
             if (state->user_fsm_command == CALIB_CMD_NEXT_STAGE) {
                printf("CALIB: Starting Y-axis sampling for %u samples.\n", p->selection_size);
                state->samples_collected_current_axis = 0;
                state->calib_stage = CALIB_STAGE_Y_AXIS_SAMPLING;
            }
            break;

        case CALIB_STAGE_Y_AXIS_SAMPLING:
            if (i->input.y < state->min_y_raw) state->min_y_raw = i->input.y;
            if (i->input.y > state->max_y_raw) state->max_y_raw = i->input.y;
            state->samples_collected_current_axis++;
            if (state->samples_collected_current_axis >= p->selection_size) {
                printf("CALIB: Y-Axis sampling complete. Min: %.4f, Max: %.4f\n", state->min_y_raw, state->max_y_raw);
                 if (fabs(state->max_y_raw - state->min_y_raw) < 1e-6) {
                    printf("CALIB_ERROR: Y-Axis min/max are too close. Restarting calibration.\n");
                    state->calib_stage = CALIB_STAGE_ERROR;
                    break;
                }
                state->calib_stage = CALIB_STAGE_Z_AXIS_INIT;
            }
            break;

        case CALIB_STAGE_Z_AXIS_INIT:
            printf("CALIB: Z-Axis Init. Send NEXT_STAGE command when ready to sample Z-axis min/max.\n");
            printf("CALIB: Rotate sensor slowly to capture full range for Z-axis.\n");
            state->samples_collected_current_axis = 0; // Reset sample count for this axis
            state->calib_stage = CALIB_STAGE_Z_AXIS_PROMPT;
            break;

        case CALIB_STAGE_Z_AXIS_PROMPT:
            if (state->user_fsm_command == CALIB_CMD_NEXT_STAGE) {
                printf("CALIB: Starting Z-axis sampling for %u samples.\n", p->selection_size);
                state->samples_collected_current_axis = 0;
                state->calib_stage = CALIB_STAGE_Z_AXIS_SAMPLING;
            }
            break;

        case CALIB_STAGE_Z_AXIS_SAMPLING:
            if (i->input.z < state->min_z_raw) state->min_z_raw = i->input.z;
            if (i->input.z > state->max_z_raw) state->max_z_raw = i->input.z;
            state->samples_collected_current_axis++;
            if (state->samples_collected_current_axis >= p->selection_size) {
                printf("CALIB: Z-Axis sampling complete. Min: %.4f, Max: %.4f\n", state->min_z_raw, state->max_z_raw);
                if (fabs(state->max_z_raw - state->min_z_raw) < 1e-6) {
                    printf("CALIB_ERROR: Z-Axis min/max are too close. Restarting calibration.\n");
                    state->calib_stage = CALIB_STAGE_ERROR;
                    break;
                }
                state->calib_stage = CALIB_STAGE_CALCULATE_SAVE;
            }
            break;

        case CALIB_STAGE_CALCULATE_SAVE:
            printf("CALIB: Calculating and saving calibration coefficients.\n");
            // Calculate Biases (midpoint of min/max)
            state->current_b.x = (state->max_x_raw + state->min_x_raw) / 2.0;
            state->current_b.y = (state->max_y_raw + state->min_y_raw) / 2.0;
            state->current_b.z = (state->max_z_raw + state->min_z_raw) / 2.0;

            // Calculate K-factors (scale factors)
            // K = TargetMagnitude / ( (Max_raw - Min_raw) / 2 )
            // Avoid division by zero if range is too small (should be caught earlier)
            double range_x = state->max_x_raw - state->min_x_raw;
            double range_y = state->max_y_raw - state->min_y_raw;
            double range_z = state->max_z_raw - state->min_z_raw;

            state->current_k.x = (fabs(range_x) > 1e-7) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_x : 1.0;
            state->current_k.y = (fabs(range_y) > 1e-7) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_y : 1.0;
            state->current_k.z = (fabs(range_z) > 1e-7) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_z : 1.0;

            printf("CALIB: Calculated B: {%.4f, %.4f, %.4f}, K: {%.4f, %.4f, %.4f}\n",
                   state->current_b.x, state->current_b.y, state->current_b.z,
                   state->current_k.x, state->current_k.y, state->current_k.z);

            save_calibration_data(p->settings_path, state);
            state->calib_stage = CALIB_STAGE_DONE_APPLYING;
            printf("CALIB: Calibration complete. Now applying new coefficients.\n");
            state->calibration_active_request = false; // Calibration process itself is done
            break;

        case CALIB_STAGE_DONE_APPLYING:
            // Consistently applying coefficients. If user wants to recalibrate,
            // they need to set calibration_active_request = true via CLI.
            // If calibration_active_request becomes true, FSM will move to X_AXIS_INIT.
             if (state->calibration_active_request) {
                printf("CALIB: Re-initiating calibration from DONE_APPLYING state.\n");
                state->calib_stage = CALIB_STAGE_X_AXIS_INIT;
            }
            break;

        case CALIB_STAGE_ERROR:
            printf("CALIB_ERROR: An error occurred. Calibration halted. Send RESTART or CANCEL command.\n");
            // Stay in this state until user issues a command
            if (state->user_fsm_command == CALIB_CMD_RESTART_CALIBRATION) { // Already handled above, but good for clarity
                 state->calib_stage = CALIB_STAGE_X_AXIS_INIT;
                 reset_min_max(state);
            } else if (state->user_fsm_command == CALIB_CMD_CANCEL_CALIBRATION) { // Already handled above
                 state->calib_stage = CALIB_STAGE_IDLE;
                 // Parameters would have been reset by the global command handler
            }
            break;

        default:
            printf("CALIB: Unknown calibration stage: %d. Resetting to IDLE.\n", state->calib_stage);
            state->calib_stage = CALIB_STAGE_IDLE;
            break;
    }
}


fspec_rv_t core_calib_v3f64_pre_exec_init(const core_calib_v3f64_params_t *p, core_calib_v3f64_state_t *state) {
    state->calib_stage = CALIB_STAGE_IDLE;
    state->iteration_counter = 0;
    state->calibration_active_request = 0;
    state->user_fsm_command = CALIB_CMD_NONE;

    // Initialize K and B from parameters first
    state->current_k.x = p->initial_k1;
    state->current_k.y = p->initial_k2;
    state->current_k.z = p->initial_k3;
    state->current_b.x = p->initial_b1;
    state->current_b.y = p->initial_b2;
    state->current_b.z = p->initial_b3;

    reset_min_max(state);

    // Attempt to load from file, potentially overriding initial params
    if (load_calibration_data(p->settings_path, state)) {
        state->calib_stage = CALIB_STAGE_DONE_APPLYING; // Loaded successfully
    } else {
        // If load fails or no file, use initial params, stay in IDLE
        // (or whatever stage is appropriate if it was not the first init)
        printf("CALIB: Using initial K={%.4f, %.4f, %.4f}, B={%.4f, %.4f, %.4f}\n",
               state->current_k.x, state->current_k.y, state->current_k.z,
               state->current_b.x, state->current_b.y, state->current_b.z);
    }

    fspec_rv_t rv = fspec_rv_ok;
    eswb_rv_t erv;
    char topic_name_buf[128]; // Buffer for topic names

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_enable, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s/enable", p->cli_base_alias);
    topic_proclaiming_tree_t *enable_cmd_root = usr_topic_set_fifo(cntx_enable, topic_name_buf, 2);
    usr_topic_add_struct_child(cntx_enable, enable_cmd_root, atomic_cmd_bool_t, cmd_bool, "cmd_bool", tt_int32);
    erv = atomic_cli_register_fifo(cntx_enable, enable_cmd_root, &state->enable_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        fprintf(stderr, "CALIB_INIT_ERROR: Failed to register enable CLI FIFO for %s (%s)\n", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_stage, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s/cmd", p->cli_base_alias);
    topic_proclaiming_tree_t *stage_cmd_root = usr_topic_set_fifo(cntx_stage, topic_name_buf, 2);
    usr_topic_add_struct_child(cntx_stage, stage_cmd_root, atomic_cmd_i32_t, cmd_i32, "cmd_i32", tt_int32);
    erv = atomic_cli_register_fifo(cntx_stage, stage_cmd_root, &state->stage_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        fprintf(stderr, "CALIB_INIT_ERROR: Failed to register stage_cmd CLI FIFO for %s (%s)\n", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    printf("CALIB: Initialization complete. Current stage: %d. Listening on CLI topics based on '%s'.\n", state->calib_stage, p->cli_base_alias);
    return rv;
}

void core_calib_v3f64_exec(
    const core_calib_v3f64_inputs_t *i,
    core_calib_v3f64_outputs_t *o,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state
) {
    // 1. Handle CLI Inputs
    atomic_cmd_bool_t enable_cmd;
    if (eswb_fifo_try_pop(state->enable_cmd_fifo_td, &enable_cmd) == eswb_e_ok) {
        if (state->calibration_active_request != enable_cmd.cmd_bool) {
            state->calibration_active_request = enable_cmd.cmd_bool;
            printf("CALIB: Calibration active request set to %s via CLI.\n", state->calibration_active_request ? "TRUE" : "FALSE");
            if (state->calibration_active_request && state->calib_stage == CALIB_STAGE_IDLE) {
                 state->calib_stage = CALIB_STAGE_X_AXIS_INIT; // Kickstart FSM if it was idle
                 reset_min_max(state); // Reset sampling data
            } else if (!state->calibration_active_request && state->calib_stage > CALIB_STAGE_IDLE && state->calib_stage < CALIB_STAGE_DONE_APPLYING) {
                printf("CALIB: Calibration disabled mid-process. Returning to IDLE, preserving current K/B.\n");
                // If disabled during active calibration (not IDLE or DONE_APPLYING), revert to IDLE
                // but keep the *current* K and B values, don't reload from file/initials immediately
                // unless a CANCEL command is issued.
                state->calib_stage = CALIB_STAGE_IDLE;
            }
        }
    }

    atomic_cmd_i32_t stage_fsm_cmd;
    if (eswb_fifo_try_pop(state->stage_cmd_fifo_td, &stage_fsm_cmd) == eswb_e_ok) {
        state->user_fsm_command = stage_fsm_cmd.cmd_i32;
        printf("CALIB: Received FSM command: %d\n", state->user_fsm_command);
    }

    // 2. Decimation and FSM Handling
    // The FSM should only run if calibration is active OR if it's in a state that needs to react to commands (like ERROR or DONE_APPLYING with a new request)
    bool fsm_can_run = state->calibration_active_request ||
                       state->calib_stage == CALIB_STAGE_ERROR || // Allow commands in error state
                       (state->calib_stage == CALIB_STAGE_DONE_APPLYING && state->user_fsm_command != CALIB_CMD_NONE) || // Allow restart from done
                       (state->user_fsm_command == CALIB_CMD_CANCEL_CALIBRATION || state->user_fsm_command == CALIB_CMD_RESTART_CALIBRATION); // Global commands

    if (fsm_can_run) {
        state->iteration_counter++;
        uint16_t decimation = (p->decimation_factor == 0) ? 1 : p->decimation_factor; // Avoid division by zero
        if (state->iteration_counter >= decimation) {
            state->iteration_counter = 0;
            handle_calibration_fsm(i, p, state);
        }
    } else if (state->calib_stage != CALIB_STAGE_IDLE && state->calib_stage != CALIB_STAGE_DONE_APPLYING) {
        // If not active and not in a stable end state, go to IDLE.
        // This handles cases where `calibration_active_request` becomes false without a CANCEL command.
        state->calib_stage = CALIB_STAGE_IDLE;
    }


    // 3. Always Apply Current Calibration Coefficients
    o->v.x = (i->input.x - state->current_b.x) * state->current_k.x;
    o->v.y = (i->input.y - state->current_b.y) * state->current_k.y;
    o->v.z = (i->input.z - state->current_b.z) * state->current_k.z;

    // 4. Calculate Output Magnitude
    o->calibrated_magnitude = sqrt(o->v.x * o->v.x + o->v.y * o->v.y + o->v.z * o->v.z);

    // 5. Output Current Stage
    o->current_stage_out = state->calib_stage;

    // 6. Reset user command after processing
    state->user_fsm_command = CALIB_CMD_NONE;
}