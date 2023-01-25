from fspeclib import *


Function(
    name='core.vector.dot_product',
    title=LocalizedString(
        en='Scalar multiplication of two vectors'
    ),

    inputs=[
        Input(
            name='v1',
            title='Vector 1',
            value_type='core.type.v3f64'
        ),
        Input(
            name='v2',
            title='Vector 2',
            value_type='core.type.v3f64'
        ),
    ],

    outputs=[
        Output(
            name='output',
            title='output',
            value_type='core.type.f64'
        )
    ],
)
