from ctlst import *


Function(
    name='core.math.acos',
    title=LocalizedString(
        en='Arccosine value of number'
    ),
    description=LocalizedString(
        en='This module returns the arccosine, or inverse cosine, of a input.'
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
