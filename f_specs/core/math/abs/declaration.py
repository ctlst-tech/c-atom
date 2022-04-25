from ctlst import *


Function(
    name='core.math.abs',
    title=LocalizedString(
        en='Absolute value of number'
    ),
    description=LocalizedString(
        en='This function returns the absolute value (i.e. the modulus) of input value.'
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
