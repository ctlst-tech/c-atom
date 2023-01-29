from fspeclib import *


Function(
    name='core.quat.conjugate',
    title=LocalizedString(
        en='Conjugate quaternion'
    ),

    inputs=[
        Input(
            name='q',
            title='Input quat',
            value_type='core.type.quat'
        ),
    ],

    outputs=[
        Output(
            name='q',
            title='Conjugated quat',
            value_type='core.type.quat'
        ),
    ],
)
