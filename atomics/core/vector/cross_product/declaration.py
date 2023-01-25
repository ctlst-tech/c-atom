from fspeclib import *


Function(
    name='core.vector.cross_product',
    title=LocalizedString(
        en='Vector multiplication'
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
            name='v',
            title='output',
            value_type='core.type.v3f64'
        )
    ],
)
