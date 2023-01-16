from fspeclib import *


Function(
    name='core.math.clamp',
    title=LocalizedString(
        en='Clamp number into range'
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
            value_type='core.type.f64'
        )
    ]
)
