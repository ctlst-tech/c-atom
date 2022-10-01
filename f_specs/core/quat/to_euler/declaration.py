from ctlst import *


Function(
    name='core.quat.to_euler',
    title=LocalizedString(
        en='Covert quaternion to Euler angles'
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
            name='roll',
            title='Roll',
            value_type='core.type.f64'
        ),
        Output(
            name='pitch',
            title='Pitch',
            value_type='core.type.f64'
        ),
        Output(
            name='yaw',
            title='Yaw',
            value_type='core.type.f64'
        ),
    ],
)
