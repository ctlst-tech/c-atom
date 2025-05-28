#include <ctype.h>
#include <float.h>  // For DBL_MAX, DBL_EPSILON
#include <math.h>
#include <stdarg.h>  // For va_list, va_start, va_end
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atomics_cli_cmd.h"  // For atomic_cmd_i32_t, atomic_cmd_bool_t
#include "core_calib_v3f64.h"
#include "eswb/api.h"
#include "eswb/topic_proclaiming_tree.h"  // For ESWB topic setup

// Calibration FSM Stages
#define CALIB_STAGE_IDLE 0
#define CALIB_STAGE_START_AXIS_SEQUENCE 5
#define CALIB_STAGE_PROMPT_EXTREME 10
#define CALIB_STAGE_SAMPLING_EXTREME 11
#define CALIB_STAGE_CALCULATE_SAVE 40
#define CALIB_STAGE_DONE_APPLYING 50
#define CALIB_STAGE_ERROR 60

// User FSM Commands (from stage_cmd_fifo_td)
#define CALIB_CMD_NONE 0
#define CALIB_CMD_NEXT_STAGE 1
#define CALIB_CMD_RESTART_CALIBRATION 2
#define CALIB_CMD_CANCEL_CALIBRATION 3

// Helper for formatted printing with alias
static void calib_printf(const char *cli_base_alias, bool is_error, const char *format, ...) {
    char prefixed_format[256];
    snprintf(prefixed_format, sizeof(prefixed_format), "CALIB [%s]: %s\n", cli_base_alias ? cli_base_alias : "NO_ALIAS", format);

    va_list args;
    va_start(args, format);
    vfprintf(is_error ? stderr : stdout, prefixed_format, args);
    va_end(args);
}

// Helper to get axis name as string
static const char* get_axis_name(uint8_t axis_idx) {
    if (axis_idx == 0) return "X";
    if (axis_idx == 1) return "Y";
    if (axis_idx == 2) return "Z";
    return "UNKNOWN";
}

// Helper to get the input value for the current sampling axis
static double get_input_for_axis(const core_type_v3f64_t *input, uint8_t axis_idx) {
    if (axis_idx == 0) return input->x;
    if (axis_idx == 1) return input->y;
    if (axis_idx == 2) return input->z;
    return 0.0;
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

static void reset_averaged_min_max_data(core_calib_v3f64_state_t *state) {
    state->min_x_raw_avg = DBL_MAX; state->max_x_raw_avg = -DBL_MAX;
    state->min_y_raw_avg = DBL_MAX; state->max_y_raw_avg = -DBL_MAX;
    state->min_z_raw_avg = DBL_MAX; state->max_z_raw_avg = -DBL_MAX;
    state->num_samples_in_accumulator = 0;
    state->current_sampling_axis_idx = 0;
    state->is_positive_extreme_next = true;
    state->current_axis_avg_extreme1 = 0.0;
}

static bool load_calibration_data(const char *cli_base_alias, const char *path,
                                  core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) return false;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        calib_printf(
            cli_base_alias, false,
            "No calibration file '%s'. Using initial/current parameters.",
            path);
        return false;
    }

    char buf[256];
    ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read <= 0) {
        calib_printf(
            cli_base_alias, true,
            "Error reading file '%s'. Using initial/current parameters.", path);
        close(fd);
        return false;
    }
    buf[bytes_read] = '\0';
    close(fd);

    char *token = buf;
    char *ptr;
    int i = 0;
    double temp[6] = {0};

    while (token && i < 6) {
        while (*token && isspace(*token)) token++;
        if (!*token) break;

        temp[i] = strtod(token, &ptr);
        if (ptr == token) break;

        i++;
        token = ptr;
    }

    state->current_b.x = temp[0];
    state->current_b.y = temp[1];
    state->current_b.z = temp[2];
    state->current_k.x = temp[3];
    state->current_k.y = temp[4];
    state->current_k.z = temp[5];

    if (i == 6) {
        calib_printf(cli_base_alias, false,
                     "Loaded K={%.4f,%.4f,%.4f}, B={%.4f,%.4f,%.4f} from '%s'",
                     state->current_k.x, state->current_k.y, state->current_k.z,
                     state->current_b.x, state->current_b.y, state->current_b.z,
                     path);
        return true;
    }

    calib_printf(cli_base_alias, true,
                 "Error parsing file '%s'. Using initial/current parameters.",
                 path);
    return false;
}

static bool save_calibration_data(const char *cli_base_alias, const char *path, const core_calib_v3f64_state_t *state) {
    if (path == NULL || strlen(path) == 0) return false;
    FILE *f = fopen(path, "w");
    if (!f) {
        calib_printf(cli_base_alias, true, "Error opening file '%s' for writing.", path);
        return false;
    }
    fprintf(f, "%.10g %.10g %.10g\n", state->current_b.x, state->current_b.y, state->current_b.z);
    fprintf(f, "%.10g %.10g %.10g\n", state->current_k.x, state->current_k.y, state->current_k.z);
    fclose(f);
    calib_printf(cli_base_alias, false, "Saved K={%.4f,%.4f,%.4f}, B={%.4f,%.4f,%.4f} to '%s'",
           state->current_k.x, state->current_k.y, state->current_k.z,
           state->current_b.x, state->current_b.y, state->current_b.z, path);
    return true;
}

static void handle_calibration_fsm(
    const core_calib_v3f64_inputs_t *i,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state,
    int32_t user_fsm_cmd) {

    // Store current stage to detect change for messages
    // This is now handled by comparing state->calib_stage with state->previous_calib_stage

    bool print_instr = (state->calib_stage != state->previous_calib_stage);
                        // || (user_fsm_cmd != CALIB_CMD_NONE && state->calib_stage == CALIB_STAGE_PROMPT_EXTREME);

    // Handle global commands that can interrupt any stage if calibration is active
    if (user_fsm_cmd == CALIB_CMD_CANCEL_CALIBRATION) {
        calib_printf(p->cli_base_alias, false, "CANCEL received. Calibration stopped and reset.");
        state->calibration_active = false; // This will be caught by the outer logic in process_calibration_logic
        state->calib_stage = CALIB_STAGE_IDLE;
        // Revert to initial or loaded K/B
        state->current_k.x = p->initial_k1; state->current_k.y = p->initial_k2; state->current_k.z = p->initial_k3;
        state->current_b.x = p->initial_b1; state->current_b.y = p->initial_b2; state->current_b.z = p->initial_b3;
        load_calibration_data(p->cli_base_alias, p->settings_path, state);
        return; // FSM processing stops here for cancel
    }
    if (user_fsm_cmd == CALIB_CMD_RESTART_CALIBRATION) {
        calib_printf(p->cli_base_alias, false, "RESTART received. Restarting calibration sequence.");
        // state->calibration_active should already be true
        reset_averaged_min_max_data(state);
        state->calib_stage = CALIB_STAGE_START_AXIS_SEQUENCE;
        print_instr = true; // Force print for the new stage
    }

    // FSM logic
    switch (state->calib_stage) {
        case CALIB_STAGE_IDLE:
            // This stage is mostly a placeholder; activation logic in process_calibration_logic moves to START_AXIS_SEQUENCE
            if (print_instr) { // Should only happen if forcefully set to IDLE and it was different before
                calib_printf(p->cli_base_alias, false, "Calibration IDLE. Enable via CLI to start.");
            }
            break;

        case CALIB_STAGE_START_AXIS_SEQUENCE:
            if (print_instr) { // Print only on first entry to this stage
                 calib_printf(p->cli_base_alias, false, "Initializing for %s-axis calibration.", get_axis_name(state->current_sampling_axis_idx));
            }
            state->is_positive_extreme_next = true;
            state->num_samples_in_accumulator = 0;
            state->calib_stage = CALIB_STAGE_PROMPT_EXTREME;
            // Fall through to print prompt in this cycle if previous_stage was different
            print_instr = true; // Force print for next stage as we are transitioning
            // No break, fall through

        case CALIB_STAGE_PROMPT_EXTREME:
            if (print_instr) { // Print only on first entry or if command makes us re-evaluate
                const char* axis_name = get_axis_name(state->current_sampling_axis_idx);
                const char* extreme_direction = state->is_positive_extreme_next ? "positive" : "negative";
                const char* example_orientation = "";
                if (strcmp(axis_name, "X") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., X-axis pointing UP" : "e.g., X-axis pointing DOWN";
                else if (strcmp(axis_name, "Y") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., Y-axis pointing UP" : "e.g., Y-axis pointing DOWN";
                else if (strcmp(axis_name, "Z") == 0) example_orientation = state->is_positive_extreme_next ? "e.g., Z-axis pointing UP" : "e.g., Z-axis pointing DOWN";

                calib_printf(p->cli_base_alias, false, "Position sensor for %s %s-axis extreme (%s) and hold steady.",
                       extreme_direction, axis_name, example_orientation);
                calib_printf(p->cli_base_alias, false, "Send NEXT_STAGE command to start sampling: atomics_cli %s_cmd %d",
                             p->cli_base_alias, CALIB_CMD_NEXT_STAGE);
            }
            if (user_fsm_cmd == CALIB_CMD_NEXT_STAGE) {
                calib_printf(p->cli_base_alias, false, "Starting %s %s-axis sampling for %u samples.",
                       state->is_positive_extreme_next ? "positive" : "negative", get_axis_name(state->current_sampling_axis_idx), p->selection_size);
                state->num_samples_in_accumulator = 0;
                if (state->sample_accumulator.vector == NULL && p->selection_size > 0) {
                     calib_printf(p->cli_base_alias, true, "Sample accumulator buffer is NULL!");
                     state->calib_stage = CALIB_STAGE_ERROR;
                     break;
                }
                state->calib_stage = CALIB_STAGE_SAMPLING_EXTREME;
            }
            break;

        case CALIB_STAGE_SAMPLING_EXTREME:
            if (state->sample_accumulator.vector == NULL && p->selection_size > 0) {
                 calib_printf(p->cli_base_alias, true, "Sample accumulator buffer is NULL during sampling!");
                 state->calib_stage = CALIB_STAGE_ERROR;
                 break;
            }
            if (state->num_samples_in_accumulator < p->selection_size && state->num_samples_in_accumulator < state->sample_accumulator.max_len) {
                state->sample_accumulator.vector[state->num_samples_in_accumulator] = get_input_for_axis(&(i->input), state->current_sampling_axis_idx);
                state->num_samples_in_accumulator++;
                 if (state->num_samples_in_accumulator % (p->selection_size/4 < 1 ? 1 : p->selection_size/4) == 0 || state->num_samples_in_accumulator == p->selection_size) {
                    // Optional: print progress during sampling
                    // calib_printf(p->cli_base_alias, false, "Sampling... %u/%u collected for %s %s-axis.",
                    //        state->num_samples_in_accumulator, p->selection_size,
                    //        state->is_positive_extreme_next ? "positive" : "negative", get_axis_name(state->current_sampling_axis_idx));
                 }
            }

            if (state->num_samples_in_accumulator >= p->selection_size) {
                double sum = 0;
                for (uint16_t k = 0; k < p->selection_size; ++k) {
                    sum += state->sample_accumulator.vector[k];
                }
                double current_average = (p->selection_size > 0) ? (sum / p->selection_size) : get_input_for_axis(&(i->input), state->current_sampling_axis_idx); // Use current if selection_size is 0

                const char* axis_name = get_axis_name(state->current_sampling_axis_idx);
                calib_printf(p->cli_base_alias, false, "%s-axis, %s extreme sampling complete. Averaged value: %.4f",
                       axis_name, state->is_positive_extreme_next ? "positive" : "negative", current_average);

                if (state->is_positive_extreme_next) {
                    state->current_axis_avg_extreme1 = current_average;
                    state->is_positive_extreme_next = false;
                    state->calib_stage = CALIB_STAGE_PROMPT_EXTREME;
                } else {
                    store_axis_min_max_avg(state, state->current_sampling_axis_idx, state->current_axis_avg_extreme1, current_average);
                    double r_min = (state->current_axis_avg_extreme1 < current_average ? state->current_axis_avg_extreme1 : current_average);
                    double r_max = (state->current_axis_avg_extreme1 > current_average ? state->current_axis_avg_extreme1 : current_average);
                    calib_printf(p->cli_base_alias, false, "%s-axis min/max averages determined: Min=%.4f, Max=%.4f.", axis_name, r_min, r_max);

                    if (fabs(r_max - r_min) < 1e-6 * fabs(p->target_calibrated_magnitude_per_axis)) {
                         calib_printf(p->cli_base_alias, true, "%s-Axis min/max averages are too close (%.4f, %.4f). Insufficient movement or sensor issue.", axis_name, r_min, r_max);
                         state->calib_stage = CALIB_STAGE_ERROR;
                         break;
                    }

                    calib_printf(p->cli_base_alias, false, "Send NEXT_STAGE command to go to next phase: atomics_cli %s_cmd %d", p->cli_base_alias, CALIB_CMD_NEXT_STAGE);

                    state->current_sampling_axis_idx++;
                    if (state->current_sampling_axis_idx < 3) {
                        calib_printf(p->cli_base_alias, false, "Advancing to %s-axis calibration.", get_axis_name(state->current_sampling_axis_idx));
                        state->is_positive_extreme_next = true;
                        state->calib_stage = CALIB_STAGE_PROMPT_EXTREME;
                    } else {
                        calib_printf(p->cli_base_alias, false, "All axes sampled. Proceeding to calculate coefficients.");
                        state->calib_stage = CALIB_STAGE_CALCULATE_SAVE;
                    }
                }
                state->num_samples_in_accumulator = 0;
            }
            break;

        case CALIB_STAGE_CALCULATE_SAVE:
            if (print_instr) { // Print only on first entry
                calib_printf(p->cli_base_alias, false, "Calculating and saving calibration coefficients using averaged min/max.");
            }
            state->current_b.x = (state->max_x_raw_avg + state->min_x_raw_avg) / 2.0;
            state->current_b.y = (state->max_y_raw_avg + state->min_y_raw_avg) / 2.0;
            state->current_b.z = (state->max_z_raw_avg + state->min_z_raw_avg) / 2.0;

            double range_x = state->max_x_raw_avg - state->min_x_raw_avg;
            double range_y = state->max_y_raw_avg - state->min_y_raw_avg;
            double range_z = state->max_z_raw_avg - state->min_z_raw_avg;

            state->current_k.x = (fabs(range_x) > DBL_EPSILON * fabs(p->target_calibrated_magnitude_per_axis)) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_x : 1.0;
            state->current_k.y = (fabs(range_y) > DBL_EPSILON * fabs(p->target_calibrated_magnitude_per_axis)) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_y : 1.0;
            state->current_k.z = (fabs(range_z) > DBL_EPSILON * fabs(p->target_calibrated_magnitude_per_axis)) ? (2.0 * p->target_calibrated_magnitude_per_axis) / range_z : 1.0;

            calib_printf(p->cli_base_alias, false, "New B: {%.4f,%.4f,%.4f}, K: {%.4f,%.4f,%.4f}",
                   state->current_b.x, state->current_b.y, state->current_b.z,
                   state->current_k.x, state->current_k.y, state->current_k.z);

            save_calibration_data(p->cli_base_alias, p->settings_path, state);
            state->calib_stage = CALIB_STAGE_DONE_APPLYING;
            // Fall through to print done message
            print_instr = true; // Force print for next stage as we are transitioning
            // No break

        case CALIB_STAGE_DONE_APPLYING:
            if (print_instr) { // Print only on first entry
                calib_printf(p->cli_base_alias, false, "Calibration complete. Applying new coefficients.");
            }
            state->calibration_active = false; // Calibration process itself is done. Stays in this stage applying coeffs.
            break;

        case CALIB_STAGE_ERROR:
            if (print_instr) { // Print only on first entry to error state
                calib_printf(p->cli_base_alias, true, "FSM in error state. Calibration halted.");
                calib_printf(p->cli_base_alias, true, "Send RESTART or CANCEL command: atomic_cli publish %s/cmd '{ \"cmd_i32\": %d }' (RESTART) or '{ \"cmd_i32\": %d }' (CANCEL)",
                             p->cli_base_alias, CALIB_CMD_RESTART_CALIBRATION, CALIB_CMD_CANCEL_CALIBRATION);
            }
            state->calibration_active = false; // Stop FSM processing unless command received
            break;

        default:
            calib_printf(p->cli_base_alias, true, "Unknown stage: %d. Resetting to IDLE.", state->calib_stage);
            state->calib_stage = CALIB_STAGE_IDLE;
            state->calibration_active = false;
            break;
    }
}

static void process_calibration_logic(
    const core_calib_v3f64_inputs_t *i,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state) {

    // 1. Handle calibration activation timeout (runs regardless of current active state, if not permanently disabled yet)
    if (p->calibration_start_timeout_iterations > 0 && !state->calibration_active) {
        state->init_iterations_counter++;
        if (state->init_iterations_counter > p->calibration_start_timeout_iterations) {
            state->calibration_disabled = true; // This will be caught by core_calib_v3f64_exec on the *next* call
            calib_printf(p->cli_base_alias, false, "Activation timeout reached. Calibration permanently disabled for this session.");
            return;
        }
    }

    // 2. Check for CLI 'enable' command
    atomic_cmd_bool_t enable_cmd_payload;
    bool prev_calibration_active = state->calibration_active;

    if (eswb_fifo_try_pop(state->enable_cmd_fifo_td, &enable_cmd_payload) == eswb_e_ok) {
        bool new_active_state = enable_cmd_payload.cmd_bool;
        if (state->calibration_active != new_active_state) {
            state->calibration_active = new_active_state;
            calib_printf(p->cli_base_alias, false, "'%s_enable' CLI: Calibration active set to %s.", p->cli_base_alias, state->calibration_active ? "TRUE" : "FALSE");

            if (state->calibration_active) { // Just became active
                // If was idle, done, or error, reset to start the sequence
                if (state->calib_stage == CALIB_STAGE_IDLE || state->calib_stage == CALIB_STAGE_DONE_APPLYING || state->calib_stage == CALIB_STAGE_ERROR) {
                    reset_averaged_min_max_data(state);
                    state->calib_stage = CALIB_STAGE_START_AXIS_SEQUENCE;
                    calib_printf(p->cli_base_alias, false, "Calibration (re)activated. Will proceed from initial axis sequence.");
                } else {
                     calib_printf(p->cli_base_alias, false, "Calibration re-activated, continuing from stage %d.", state->calib_stage);
                }
            } else { // Just became inactive
                calib_printf(p->cli_base_alias, false, "Calibration explicitly disabled by CLI. Halting FSM.");
                if (state->calib_stage > CALIB_STAGE_IDLE && state->calib_stage < CALIB_STAGE_DONE_APPLYING && state->calib_stage != CALIB_STAGE_ERROR) {
                    calib_printf(p->cli_base_alias, false, "Calibration was in progress. Stage reset to IDLE.");
                }
                state->calib_stage = CALIB_STAGE_IDLE; // Reset to IDLE when disabled.
            }
        }
    }

    // 3. If calibration is active, process FSM and 'cmd' commands
    if (state->calibration_active || state->calib_stage == CALIB_STAGE_ERROR) { // Allow CMDs in error state to recover
        int32_t user_fsm_cmd = CALIB_CMD_NONE;
        atomic_cmd_i32_t stage_fsm_payload;

        if (eswb_fifo_try_pop(state->stage_cmd_fifo_td, &stage_fsm_payload) == eswb_e_ok) {
            user_fsm_cmd = stage_fsm_payload.cmd_i32;
            calib_printf(p->cli_base_alias, false, "'%s_cmd' CLI: Received FSM command: %d", p->cli_base_alias, user_fsm_cmd);
        }

        // Execute FSM based on decimation or if a command is received
        state->fsm_iteration_counter++;
        uint16_t decimation = (p->decimation_factor == 0) ? 1 : p->decimation_factor;

        // Process FSM if:
        // - It's active and decimation count is met OR
        // - A user command was received (process immediately) OR
        // - Calibration just became active (process first step immediately)
        if ( (state->calibration_active && state->fsm_iteration_counter >= decimation) ||
             user_fsm_cmd != CALIB_CMD_NONE ||
             (state->calibration_active && !prev_calibration_active) ) {
            state->fsm_iteration_counter = 0;
            handle_calibration_fsm(i, p, state, user_fsm_cmd);
        }
    }
    // Update previous_calib_stage *after* all FSM logic for the current cycle has completed
    state->previous_calib_stage = state->calib_stage;
}

// Initialization function
fspec_rv_t core_calib_v3f64_pre_exec_init(const core_calib_v3f64_params_t *p, core_calib_v3f64_state_t *state) {
    state->calib_stage = CALIB_STAGE_IDLE;
    state->previous_calib_stage = -1; // Ensure first message prints
    state->calibration_active = false;
    state->fsm_iteration_counter = 0;

    state->current_k.x = p->initial_k1; state->current_k.y = p->initial_k2; state->current_k.z = p->initial_k3;
    state->current_b.x = p->initial_b1; state->current_b.y = p->initial_b2; state->current_b.z = p->initial_b3;

    reset_averaged_min_max_data(state);

    state->init_iterations_counter = 0;
    state->calibration_disabled = false;

    if (p->selection_size == 0) {
        calib_printf(p->cli_base_alias, false, "Warning: selection_size is 0. Averaging will use single current sample.");
    }
    calib_printf(p->cli_base_alias, false, "Sample accumulator max_len (from selection_size parameter): %u", state->sample_accumulator.max_len);
     if (p->selection_size > state->sample_accumulator.max_len) {
        calib_printf(p->cli_base_alias, true, "Error: selection_size (%u) > sample_accumulator.max_len (%u). Check FSpec setup.",
                     p->selection_size, state->sample_accumulator.max_len);
        // Depending on strictness, could return fspec_rv_initerr here
    }


    if (load_calibration_data(p->cli_base_alias, p->settings_path, state)) {
        state->calib_stage = CALIB_STAGE_DONE_APPLYING; // Start by applying loaded coeffs
        state->previous_calib_stage = -1; // Ensure "applying new coeffs" message if relevant
        calib_printf(p->cli_base_alias, false, "INIT: Loaded calibration from '%s'. Stage set to DONE_APPLYING.", p->settings_path);
    } else {
        calib_printf(p->cli_base_alias, false, "INIT: Using initial K,B. Stage set to IDLE.");
    }

    // Setup ESWB CLI FIFOs
    fspec_rv_t rv = fspec_rv_ok;
    eswb_rv_t erv;
    char topic_name_buf[128];

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_enable, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s_enable", p->cli_base_alias);
    topic_proclaiming_tree_t *enable_cmd_root = usr_topic_set_fifo(cntx_enable, topic_name_buf, 2); // FIFO size 2
    usr_topic_add_struct_child(cntx_enable, enable_cmd_root, atomic_cmd_bool_t, cmd_bool, "cmd_bool", tt_bool);
    erv = atomic_cli_register_fifo(topic_name_buf, cntx_enable, enable_cmd_root, &state->enable_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        calib_printf(p->cli_base_alias, true, "INIT_ERROR: Registering enable CLI for '%s' (%s)", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx_stage, 2);
    snprintf(topic_name_buf, sizeof(topic_name_buf), "%s_cmd", p->cli_base_alias);
    topic_proclaiming_tree_t *stage_cmd_root = usr_topic_set_fifo(cntx_stage, topic_name_buf, 2); // FIFO size 2
    usr_topic_add_struct_child(cntx_stage, stage_cmd_root, atomic_cmd_i32_t, cmd_i32, "cmd_i32", tt_int32);
    erv = atomic_cli_register_fifo(topic_name_buf, cntx_stage, stage_cmd_root, &state->stage_cmd_fifo_td);
    if (erv != eswb_e_ok) {
        calib_printf(p->cli_base_alias, true, "INIT_ERROR: Registering stage_cmd CLI for '%s' (%s)", topic_name_buf, eswb_strerror(erv));
        rv = fspec_rv_initerr;
    }

    return rv;
}

// Main execution function
void core_calib_v3f64_exec(
    const core_calib_v3f64_inputs_t *i,
    core_calib_v3f64_outputs_t *o,
    const core_calib_v3f64_params_t *p,
    core_calib_v3f64_state_t *state
) {
    // If calibration is permanently disabled for this session, skip all processing logic.
    // The flag is set by process_calibration_logic, so this check catches it on subsequent calls.
    if (state->calibration_disabled) {
        // Coefficients are already at their last known good state (or initial).
        // No FSM or command processing.
    } else {
        // Otherwise, run the main calibration logic.
        process_calibration_logic(i, p, state);
    }

    // 2. Always Apply Current Calibration Coefficients to the output
    o->v.x = (i->input.x - state->current_b.x) * state->current_k.x;
    o->v.y = (i->input.y - state->current_b.y) * state->current_k.y;
    o->v.z = (i->input.z - state->current_b.z) * state->current_k.z;
}
