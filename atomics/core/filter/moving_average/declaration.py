from fspeclib import *


Function(
    name='core.filter.moving_average',
    title="Moving average",
    parameters=[
        Parameter(
            name='selection_size',
            title='Selection size',
            value_type='core.type.u16',
            tunable=False,
            default=10,
            constraints=[
                ThisValue() >= 1,
                ThisValue() <= 1024
            ]
        ),
    ],
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64',
            mandatory=True,
        ),
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64',
        ),
    ],
    state=[
        Variable(
            name='selection',
            title='Selection',
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=ParameterRef('selection_size')),
        ),
        Variable(
            name='index',
            title='Selection index',
            value_type='core.type.u32',
        ),
    ],
    parameter_constraints=[],
)
