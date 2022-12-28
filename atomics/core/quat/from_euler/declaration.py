from fspeclib import *


Function(
    name='core.quat.from_euler',
    title=LocalizedString(
        en='Covert Euler angles to quaternion'
    ),
    inputs=[
        Input(
            name='roll',
            title='Roll',
            value_type='core.type.f64'
        ),
        Input(
            name='pitch',
            title='Pitch',
            value_type='core.type.f64'
        ),
        Input(
            name='yaw',
            title='Yaw',
            value_type='core.type.f64'
        ),
    ],
    outputs=[
        Output(
            name='q',
            title='Converted quat',
            value_type='core.type.quat'
        ),
    ],
)
