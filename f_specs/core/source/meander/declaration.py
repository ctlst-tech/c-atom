from fspeclib import *

Function(
    name='core.source.meander',
    title=LocalizedString(
        en='Meander with constant period'
    ),
    description=None,
    parameters=[
        Parameter(
            name='semi_period',
            title='Meadner semi period in cycles (calls number)',
            value_type='core.type.u32',
            constraints=[
                ThisValue() > 0
            ]
        ),
    ],
    outputs=[
        Output(
            name='output',
            title='Meander generator output',
            value_type='core.type.f64'
        )
    ],
    state=[
        Variable(
            name='counter',
            title='Meander counter',
            value_type='core.type.u32'
        ),
        Variable(
            name='state',
            title='Meander state',
            value_type='core.type.bool'
        )
    ]
)
