from fspeclib import *





Function(
    name='core.quat.euler_correction',
    title=LocalizedString(
        en='Calculate correction quaternion from Euler angles errors'
    ),
    inputs=[
        Input(
            name='roll_err',
            title='Roll',
            value_type='core.type.f64'
        ),
        Input(
            name='pitch_err',
            title='Pitch',
            value_type='core.type.f64'
        ),
        Input(
            name='yaw_err',
            title='Yaw',
            value_type='core.type.f64'
        ),

        Input(
            name='q',
            title='Quaternion to correct',
            value_type='core.type.quat'
        ),

        Input(
            name='roll',
            title='Current roll',
            value_type='core.type.f64'
        ),

    ],
    outputs=[
        Output(
            name='q',
            title='Correction quaternion',
            value_type='core.type.quat'
        ),
    ],
)
