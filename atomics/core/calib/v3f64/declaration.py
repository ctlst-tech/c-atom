from fspeclib import *

Function(
    name='core.calib.v3f64',
    title="3-Axis Vector Calibration with Averaged Sampling and Activation Timeout",
    has_pre_exec_init_call=True,
    parameters=[
        Parameter(
            name='selection_size',
            title='Calibration Samples per Averaging Window',
            value_type='core.type.u16',
            tunable=False,
            default=128,
            description="Number of samples to collect and average at each extreme position for an axis."
        ),
        Parameter(
            name='settings_path',
            title='Calibration Settings File Path',
            value_type='core.type.str',
            tunable=False,
            default="",
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
            title='Decimation Factor for FSM Processing',
            value_type='core.type.u16',
            default=1,
            tunable=False,
            description="Process calibration FSM logic every N calls to this function when active. 1 means process every call. Affects data sampling rate during averaging."
        ),
        Parameter(
            name='target_calibrated_magnitude_per_axis',
            title='Target Calibrated Magnitude per Axis',
            value_type='core.type.f64',
            default=1.0,
            tunable=False,
            description="The target peak magnitude for each axis after calibration (e.g., 1.0 for +/-1g). Used to calculate K factors from averaged min/max."
        ),
        Parameter(
            name='cli_base_alias',
            title='CLI Base Alias for Calibration Commands',
            value_type='core.type.str',
            default='sensor_calib',
            tunable=False,
            description="Base alias for CLI topics. Actual topics will be '[alias]/enable' (bool) and '[alias]/cmd' (i32)."
        ),
        Parameter(
            name='calibration_start_timeout_iterations',
            title='Calibration Activation Timeout (Iterations)',
            value_type='core.type.u32',
            default=0,
            tunable=False,
            description="Number of exec calls after init during which calibration can be activated via CLI. After this period, if not active, it becomes permanently disabled. Set to 0 to disable timeout."
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
            title='Output vector (calibrated sensor data)',
            value_type='core.type.v3f64',
        ),
    ],

    state=[
        Variable(
            name='calib_stage',
            title='Current Calibration FSM Stage',
            value_type='core.type.i32',
        ),
        Variable(
            name='previous_calib_stage',
            title='Previous Calibration FSM Stage',
            value_type='core.type.i32',
        ),
        Variable(
            name='enable_cmd_fifo_td',
            title='ESWB TD for Enable/Disable Calibration Command FIFO',
            value_type='core.type.td',
        ),
        Variable(
            name='stage_cmd_fifo_td',
            title='ESWB TD for Stage Progression Command FIFO',
            value_type='core.type.td',
        ),
        Variable(
            name='calibration_active',
            title='Flag: Calibration process is currently active',
            value_type='core.type.bool',
        ),
        Variable(
            name='fsm_iteration_counter',
            title='Iteration Counter for FSM Decimation',
            value_type='core.type.u32',
        ),
        # Variables for the new averaging sampling method
        Variable(
            name='sample_accumulator',
            title='Buffer to accumulate samples for averaging',
            # This assumes core.type.vector_f64 is defined similarly to core.type.vector_u8
            # and the fspec system will handle its allocation based on 'selection_size'
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=ParameterRef('selection_size'))
        ),
        Variable(
            name='num_samples_in_accumulator',
            title='Number of samples currently in the accumulator for averaging',
            value_type='core.type.u16',
        ),
        Variable(
            name='current_sampling_axis_idx', # 0:X, 1:Y, 2:Z
            title='Index of the axis currently being calibrated (0=X, 1=Y, 2=Z)',
            value_type='core.type.u8',
        ),
        Variable(
            name='is_positive_extreme_next', # True if next sampling is for positive side, false for negative
            title='Flag: Next sampling is for the positive (first) extreme of the current axis',
            value_type='core.type.bool',
        ),
        Variable(
            name='current_axis_avg_extreme1', # Stores the average from the first extreme (e.g. positive)
            title='Average value from the first sampled extreme for the current axis',
            value_type='core.type.f64',
        ),
        # Storing the determined min/max raw values (derived from averages)
        Variable(
            name='min_x_raw_avg',
            title='Averaged Minimum Raw Value on X-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_x_raw_avg',
            title='Averaged Maximum Raw Value on X-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='min_y_raw_avg',
            title='Averaged Minimum Raw Value on Y-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_y_raw_avg',
            title='Averaged Maximum Raw Value on Y-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='min_z_raw_avg',
            title='Averaged Minimum Raw Value on Z-axis',
            value_type='core.type.f64',
        ),
        Variable(
            name='max_z_raw_avg',
            title='Averaged Maximum Raw Value on Z-axis',
            value_type='core.type.f64',
        ),
        # Current K and B coefficients
        Variable(
            name='current_k',
            title='Currently Applied K factors (Kx, Ky, Kz)',
            value_type='core.type.v3f64',
        ),
        Variable(
            name='current_b',
            title='Currently Applied B biases (Bx, By, Bz)',
            value_type='core.type.v3f64',
        ),
        # Timeout logic state
        Variable(
            name='init_iterations_counter',
            title='Counter for Calibration Activation Window Post-Initialization',
            value_type='core.type.u32',
        ),
        Variable(
            name='calibration_disabled',
            title='Flag: True if Activation Window Passed and Calibration Was Not Active',
            value_type='core.type.bool',
        )
    ],
    parameter_constraints=[],
    target_link_libraries=['atomics-cli-static'],
    description="Performs 3-axis sensor calibration by averaging samples at user-fixated extreme positions for each axis. " \
                "Uses CLI commands ('[cli_base_alias]/enable', '[cli_base_alias]/cmd') and has an optional activation timeout. " \
                "Calculated coefficients are saved (if path provided) and applied. Always applies current K/B coefficients."
)