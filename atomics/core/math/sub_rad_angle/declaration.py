from fspeclib import *


Function(
    name='core.math.sub_rad_angle',
    title=LocalizedString(
        en='Subtraction of two angles in radians'
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