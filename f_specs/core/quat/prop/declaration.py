from fspeclib import *


Function(
    name='core.quat.prop',
    title=LocalizedString(
        en='Propagate quaternion'
    ),
    parameters=[

    ],
    inputs=[
        Input(
            name='wx',
            title='Angrate X',
            value_type='core.type.f64'
        ),
        Input(
            name='wy',
            title='Angrate Y',
            value_type='core.type.f64'
        ),
        Input(
            name='wz',
            title='Angrate Z',
            value_type='core.type.f64'
        ),

        Input(
            name='q0',
            title='Initial quat',
            value_type='core.type.quat'
        ),

        Input(
            name='q',
            title='Recurrent quat',
            value_type='core.type.quat'
        ),
    ],
    outputs=[
        Output(
            name='q',
            title='Updated quat',
            value_type='core.type.quat'
        ),
    ],
    state=[
        Variable(
            name='inited',
            title='Integral part',
            value_type='core.type.u32'
        ),
    ],

    injection=Injection(
        timedelta=True
    )
)
