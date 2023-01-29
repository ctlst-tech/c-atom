from fspeclib import *


Function(
    name='core.math.rad2deg',
    title=LocalizedString(
        en='Convert radians to degrees'
    ),
    description=LocalizedString(
        en='Convert degrees to radians'
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
