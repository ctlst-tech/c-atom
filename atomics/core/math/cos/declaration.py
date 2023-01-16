from fspeclib import *


Function(
    name='core.math.cos',
    title=LocalizedString(
        en='Cosine value of number'
    ),
    description=LocalizedString(
        en='This module returns the sine of a input.'
    ),
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
