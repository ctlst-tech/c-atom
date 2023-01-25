from fspeclib import *


Function(
    name='core.vector.add',
    title=LocalizedString(
        en='Sum of two vectors'
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
            title='Output vector',
            value_type='core.type.v3f64'
        ),
    ],
)
