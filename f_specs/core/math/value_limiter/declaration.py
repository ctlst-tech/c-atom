from ctlst import *


Function(
    name='core.math.value_limiter',
    title=LocalizedString(
        en='Value limiter'
    ),
    parameters=[
        Parameter(
            name='min',
            title='Lower bound',
            value_type='core.type.f64',
            tunable=True
        ),
        Parameter(
            name='max',
            title='Upper bound',
            value_type='core.type.f64',
            tunable=True
        )
    ],
    parameter_constraints=[
        ParameterValue('min') < ParameterValue('max')
    ],
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        )
    ]
)
