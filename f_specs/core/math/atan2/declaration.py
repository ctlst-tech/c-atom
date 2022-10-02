from ctlst import *


Function(
    name='core.math.atan2',
    title=LocalizedString(
        en='Arc-tangent value for 2 axis'
    ),

    inputs=[
        Input(
            name='input0',
            title='Input 1',
            value_type='core.type.f64'
        ),
        Input(
            name='input1',
            title='Input 2',
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
