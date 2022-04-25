from ctlst import *


Function(
    name='core.math.add',
    title=LocalizedString(
        en='Addition of two numbers'
    ),
    description=None,
    inputs=[
        Input(
            name='input0',
            title='Input 0',
            value_type='core.type.f64'
        ),
        Input(
            name='input1',
            title='Input 1',
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
