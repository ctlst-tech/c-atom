from ctlst import *


Function(
    name='core.nav.att_from_accel',
    title=LocalizedString(
        en='Calculate attitude based on accelerometers values'
    ),

    inputs=[
        Input(
            name='a',
            title='Input acceleration vector',
            value_type='core.type.vector3f64'
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
    ],
)
