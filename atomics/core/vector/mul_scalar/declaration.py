from fspeclib import *


Function(
    name='core.vector.mul_scalar',
    title=LocalizedString(
        en='Multiply vector by scalar'
    ),

    inputs=[
        Input(
            name='v',
            title='Vector',
            value_type='core.type.v3f64'
        ),
        Input(
            name='scalar',
            title='Vector 2',
            value_type='core.type.f64'
        ),
    ],

    outputs=[
        Output(
            name='v',
            title='output',
            value_type='core.type.v3f64'
        )
    ],
)
