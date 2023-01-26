from fspeclib import *


Function(
    name='core.vector.magnitude',
    title=LocalizedString(
        en='Vector magnitude'
    ),

    inputs=[
        Input(
            name='v',
            title='Vector',
            value_type='core.type.v3f64'
        ),
    ],

    outputs=[
        Output(
            name='output',
            title='Magnitude',
            value_type='core.type.f64'
        ),
    ],
)
