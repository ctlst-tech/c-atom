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
            name='omega',
            title='Angular rate vector',
            value_type='core.type.v3f64'
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

        Input(
            name='reset',
            title='Reset',
            description='Command for re-initializing output quat by q0',
            value_type='core.type.bool',
            mandatory=False
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
            title='Initialized flag',
            value_type='core.type.bool'
        ),
    ],

    injection=Injection(
        timedelta=True
    )
)
