from fspeclib import *


Function(
    name='core.vector.qrotate',
    title=LocalizedString(
        en='Vector rotation by quaternion'
    ),

    inputs=[
        Input(
            name='v',
            title='Input vector',
            value_type='core.type.v3f64'
        ),
        Input(
            name='q',
            title='Input quat',
            value_type='core.type.quat'
        ),
    ],

    outputs=[
        Output(
            name='v',
            title='v',
            value_type='core.type.v3f64'
        )
    ],
)
