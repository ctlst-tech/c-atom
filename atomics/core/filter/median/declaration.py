from fspeclib import *


Function(
    name='core.filter.median',
    title="Median filter for f64 values",
    parameters=[
        Parameter(
            name='selection_size',
            title='Filter selection size',
            value_type='core.type.u16',
            tunable=False,
            default=5,
            constraints=[
                ThisValue() >= 1,
                ThisValue() <= 128
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
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=ParameterRef('selection_size')),
        ),
        Variable(
            name='sorted_accumulator',
            title='Sorted Accumulator',
            value_type=VectorTypeRef(vector_type_name='core.type.vector_f64', size=ParameterRef('selection_size')),
        ), # just have it allocated in state for convenience for not bothering the stack
        Variable(
            name='head_index',
            title='Head index',
            value_type='core.type.u32',
        ),
    ],
    parameter_constraints=[],
)
