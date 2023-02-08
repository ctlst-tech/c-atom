from fspeclib import *


Function(
    name='core.vector.limiter',
    title=LocalizedString(
        en='Vector components limiter'
    ),
    parameters=[
        Parameter(
            name='max',
            title='Max',
            value_type='core.type.f64',
            tunable=True,
            default=1.0,
        ),
        Parameter(
            name='min',
            title='Min',
            value_type='core.type.f64',
            tunable=True,
            default=-1.0,
        ),
    ],
    parameter_constraints=[
        ParameterValue('min') < ParameterValue('max')
    ],
    inputs=[
        Input(
            name='v',
            title='Input vector',
            value_type='core.type.v3f64'
        ),
    ],
    outputs=[
        Output(
            name='v',
            title='Output vector',
            value_type='core.type.v3f64'
        ),
    ],
)
