from fspeclib import *


Function(
    name='core.filter.delay',
    title="Signal delay",
    parameters=[
        Parameter(
            name='cycles_to_delay',
            title='Cycles to delay number',
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
            name='accumulator',
            title='Accumulator',
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=ParameterRef('cycles_to_delay')),
        ),
        Variable(
            name='head_index',
            title='Head index',
            value_type='core.type.u32',
        ),
    ],
    parameter_constraints=[],
)
