from fspeclib import *


Function(
    name='core.vector.propagate',
    title=LocalizedString(
        en='Propagate vector with derivative over time'
    ),

    inputs=[
        Input(
            name='v',
            title='V',
            value_type='core.type.v3f64'
        ),
        Input(
            name='v0',
            title='V0',
            value_type='core.type.v3f64'
        ),

        Input(
            name='derivative',
            title='Derivative',
            value_type='core.type.v3f64'
        ),

        Input(
            name='enable',
            title='Enable',
            value_type='core.type.bool',
            mandatory=False
        ),
    ],

    state=[
        Variable(
            name='inited',
            title='Initial value is set',
            value_type='core.type.bool'
        ),
    ],

    outputs=[
        Output(
            name='v',
            title='Output vector',
            value_type='core.type.v3f64'
        ),
    ],

    injection=Injection(
        timedelta=True
    )
)
