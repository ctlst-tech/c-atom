from ctlst import *


Function(
    name='core.math.in_range',
    title=LocalizedString(
        en='Check that number is in range'
    ),
    description=None,
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        ),
        Input(
            name='min',
            title='Minimum value',
            value_type='core.type.f64'
        ),
        Input(
            name='max',
            title='Maximum value',
            value_type='core.type.f64'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.bool'
        )
    ]
)
