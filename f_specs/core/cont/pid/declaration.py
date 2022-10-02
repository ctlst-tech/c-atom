from ctlst import *


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
            constraints=[
                ThisValue() <= 0
            ]
        ),
        Parameter(
            name='integral_max',
            title='Upper bound of integral part of error',
            value_type='core.type.f64',
            tunable=True,
            default=1,
            constraints=[
                ThisValue() >= 0
            ]
        ),
        Parameter(
            name='output_min',
            title='Lower bound of controller output',
            value_type='core.type.f64',
            tunable=True,
            default=0,
            constraints=[
                ThisValue() <= 0
            ]
        ),
        Parameter(
            name='output_max',
            title='Upper bound of controller output',
            value_type='core.type.f64',
            tunable=True,
            default=1,
            constraints=[
                ThisValue() >= 0,
            ]
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
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        ),
        Output(
            name='enable',
            title='Enable',
            value_type='core.type.bool',
        )
    ],
    state=[
        Variable(
            name='integral_part',
            title='Integral part',
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
