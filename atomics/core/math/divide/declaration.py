from fspeclib import *


Function(
    name='core.math.divide',
    title=LocalizedString(
        en='Divide two numbers'
    ),
    description=None,
    inputs=[
        Input(
            name='dividend',
            title='Dividend',
            value_type='core.type.f64'
        ),
        Input(
            name='divisior',
            title='Divisior',
            value_type='core.type.f64'
        )
    ],
    parameters=[
        Parameter(
            name='min_divisior',
            title='Minimum divisior',
            value_type='core.type.f64',
            tunable=False,
            default=0.000001
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
