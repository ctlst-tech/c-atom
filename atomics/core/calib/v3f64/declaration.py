from tkinter import Variable

from fspeclib import *

Function(
    name='core.calib.v3f64',
    title="3-Axis Vector Calibration",
    has_pre_exec_init_call=True,
    parameters=[
        Parameter(
            name='selection_size',
            title='Calibration Samples per Axis',
            value_type='core.type.u16',
            tunable=False,
            default=128,
            description="Number of samples to collect for min/max determination on each axis."
        ),
        Parameter(
            name='settings_path',
            title='Calibration Settings File Path',
            value_type='core.type.str',
            tunable=False,
            description="Path to the file for loading/saving calibration K and B coefficients. Leave empty to disable file load/save."
        ),
        Parameter(
            name='initial_k1',
            title='Initial K1 factor (X-axis)',
            value_type='core.type.f64',
            default=1.0,
            tunable=False,
        ),
        Parameter(
            name='initial_k2',
            title='Initial K2 factor (Y-axis)',
            value_type='core.type.f64',
            default=1.0,
            tunable=False,
        ),
        Parameter(
            name='initial_k3',
            title='Initial K3 factor (Z-axis)',
            value_type='core.type.f64',
            default=1.0,
            tunable=False,
        ),
        Parameter(
            name='initial_b1',
            title='Initial Bias 1 (X-axis)',
            value_type='core.type.f64',
            default=0.0,
            tunable=False,
        ),
        Parameter(
            name='initial_b2',
            title='Initial Bias 2 (Y-axis)',
            value_type='core.type.f64',
            default=0.0,
            tunable=False,
        ),
        Parameter(
            name='initial_b3',
            title='Initial Bias 3 (Z-axis)',
            value_type='core.type.f64',
            default=0.0,
            tunable=False,
        ),
        Parameter(
            name='decimation_factor',
            title='Decimation Factor for FSM',
            value_type='core.type.u16',
            default=1, # Process FSM every call by default
            tunable=False,
            description="Process calibration FSM logic every N calls to this function. 1 means process every call. Affects data sampling rate during calibration."
        ),
        Parameter(
            name='target_calibrated_magnitude_per_axis',
            title='Target Calibrated Magnitude per Axis',
            value_type='core.type.f64',
            default=1.0,
            tunable=False,
            description="The target peak magnitude for each axis after calibration (e.g., 1.0 for +/-1g). Used to calculate K factors."
        ),
        Parameter(
            name='cli_base_alias',
            title='CLI Base Alias',
            value_type='core.type.str',
            default='sensor_calib', # Example: 'imu_calib'
            tunable=False,
            description="Base alias for CLI topics. Actual topics will be '[alias]/enable' (bool) and '[alias]/cmd' (i32)."
        ),
    ],

    inputs=[
        Input(
            name='input',
            title='Input vector (raw sensor data)',
            value_type='core.type.v3f64',
            mandatory=True
        ),
    ],

    outputs=[
        Output(
            name='v',
            title='Output vector (calibrated)',
            value_type='core.type.v3f64',
        ),
        Output(
            name='calibrated_magnitude',
            title='Calibrated Vector Magnitude',
            value_type='core.type.f64',
        ),
        Output(
            name='current_stage_out', # To observe FSM state
            title='Current Calibration Stage',
            value_type='core.type.i32'
        )
    ],

    state=[
        Variable(
            name='calib_stage',
            title='Calibration FSM Stage',
            value_type='core.type.i32',
        ),
        Variable(
            name='enable_cmd_fifo_td',
            title='Enable/Disable Calibration Command FIFO TD',
            value_type='core.type.td'
        ),
        Variable(
            name='stage_cmd_fifo_td',
            title='Stage Progression Command FIFO TD',
            value_type='core.type.td',
        ),
        Variable(
            name='calibration_active_request', # Controlled by enable_cmd_fifo
            title='Calibration Active Request',
            value_type='core.type.bool',
        ),
        Variable(
            name='user_fsm_command', # Last command from stage_cmd_fifo, reset after processing
            title='User FSM Command',
            value_type='core.type.i32',
        ),
        Variable(
            name='iteration_counter', # For decimation of FSM processing
            title='Iteration Counter for Decimation',
            value_type='core.type.u32',
        ),
        Variable(
            name='samples_collected_current_axis',
            title='Samples Collected for Current Axis Min/Max',
            value_type='core.type.u16',
        ),
        # Store min/max for X, Y, Z axes during their respective sampling stages
        Variable(
            name='min_x_raw',
            title='Minimum Raw Value on X-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_x_raw',
            title='Maximum Raw Value on X-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='min_y_raw',
            title='Minimum Raw Value on Y-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_y_raw',
            title='Maximum Raw Value on Y-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='min_z_raw',
            title='Minimum Raw Value on Z-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_z_raw',
            title='Maximum Raw Value on Z-axis',
            value_type='core.type.f64',
        ),
        Variable(  # Store the K and B values currently being used for correction
            name='current_k',
            title='Current K factors (Kx, Ky, Kz)',
            value_type='core.type.v3f64',
        ),
        Variable(
            name='current_b',
            title='Current B biases (Bx, By, Bz)',
            value_type='core.type.v3f64',
        )
    ],
    parameter_constraints=[],
    target_link_libraries=['atomics-cli-static'],
    description="Performs 3-axis sensor calibration (bias and scale factor). " \
                "Uses CLI commands ('[cli_base_alias]/enable' and '[cli_base_alias]/cmd') to control the calibration process. " \
                "Calibration involves prompting the user to orient the sensor to find min/max raw values for each axis sequentially. " \
                "Calculated coefficients are saved to a file (if 'settings_path' is provided) and applied to the input vector. " \
                "The block always applies the current_k and current_b coefficients. The calibration FSM updates these when new values are calculated or loaded."
)
