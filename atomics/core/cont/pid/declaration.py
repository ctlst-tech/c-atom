from fspeclib import *


Function(
    name='core.cont.pid',
    title=LocalizedString(
        en='PID controller'
    ),
    parameters=[
        Parameter(
            name='Kp',
            title='Proportional coefficient',
            value_type='core.type.f64',
            tunable=True,
            default=1,
            constraints=[
                ThisValue() >= 0
            ]
        ),
        Parameter(
            name='Ki',
            title='Integral coefficient',
            value_type='core.type.f64',
            tunable=True,
            default=0,
            constraints=[
                ThisValue() >= 0
            ]
        ),
        Parameter(
            name='Kd',
            title='Derivative coefficient',
            value_type='core.type.f64',
            tunable=True,
            default=0,
            constraints=[
                ThisValue() >= 0
            ]
        ),
        Parameter(
            name='integral_min',
            title='Lower bound of integral part of error',
            value_type='core.type.f64',
            tunable=True,
            default=0,
        ),
        Parameter(
            name='integral_max',
            title='Upper bound of integral part of error',
            value_type='core.type.f64',
            tunable=True,
            default=1,
        ),
        Parameter(
            name='output_min',
            title='Lower bound of controller output',
            value_type='core.type.f64',
            tunable=True,
            default=0,
        ),
        Parameter(
            name='output_max',
            title='Upper bound of controller output',
            value_type='core.type.f64',
            tunable=True,
            default=1,
        ),
    ],
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        ),
        Input(
            name='feedback',
            title='Feedback',
            value_type='core.type.f64'
        ),
        Input(
            name='enable',
            title='Enable',
            value_type='core.type.bool',
            mandatory=False
        ),
        Input(
            name='preset',
            title='Preset source',
            description='Used to preset integral part at the moment of the PID enabling',
            value_type='core.type.f64',
            mandatory=False
        ),
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        ),
        Output(
            name='enabled',
            title='Enabled',
            value_type='core.type.bool',
        )
    ],
    state=[
        Variable(
            name='integral',
            title='Accumulated Integral term',
            value_type='core.type.f64'
        ),
        Variable(
            name='previous_error',
            title='Previous error',
            value_type='core.type.f64'
        ),
        Variable(
            name='time_from_last_iteration',
            title='Time from last iteration',
            value_type='core.type.f64'
        ),
        Variable(
            name='activated',
            title='PID was activated before by enable signal',
            value_type='core.type.bool'
        )
    ],
    parameter_constraints=[
        ParameterValue('output_max') > ParameterValue('output_min'),
        ParameterValue('integral_max') > ParameterValue('integral_min')
    ],
    injection=Injection(
        timedelta=True
    )
)
