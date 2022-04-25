from ctlst import *


Function(
    name='core.math.neg',
    title=LocalizedString(
        en='Negative number'
    ),
    description=LocalizedString(
        en='This function returns negative of input value'
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
