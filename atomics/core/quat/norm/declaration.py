from fspeclib import *


Function(
    name='core.quat.norm',
    title=LocalizedString(
        en='Normalize quaternion'
    ),

    inputs=[
        Input(
            name='q',
            title='Input quat',
            value_type='core.type.quat'
        ),
    ],

    outputs=[
        Input(
            name='q',
            title='Normalized quat',
            value_type='core.type.quat'
        ),
    ],
)
