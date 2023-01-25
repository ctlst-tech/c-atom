from fspeclib import *


Function(
    name='core.vector.proj',
    title=LocalizedString(
        en='Vector projection on XY-plane'
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
            name='m',
            title='Vector module',
            value_type='core.type.f64'
        ),
        Output(
            name='a',
            title='Vector angle',
            value_type='core.type.f64'
        ),
    ],
)
