from fspeclib import *


Function(
    name='core.math.polynomial',
    title=LocalizedString(
        en='Polynomial'
    ),
    description=LocalizedString(
        en='Input is substituted in polynomial expression'
    ),
    has_pre_exec_init_call=True,
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        )
    ],
    parameters=[
        Parameter(
            name='coeffs',
            title='Coefficients from low to high',
            value_type='core.type.str',
            tunable=False
        ),
    ],
    state=[
        Variable(
            name='coeffs',
            title='Coeffs',
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=48),
        ),
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        )
    ]
)
