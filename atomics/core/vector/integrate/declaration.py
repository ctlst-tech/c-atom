from fspeclib import *


Function(
    name='core.vector.integrate',
    title=LocalizedString(
        en='Vector integration'
    ),

    inputs=[
        Input(
            name='v',
            title='Input vector',
            value_type='core.type.v3f64'
        ),

        Input(
            name='reset',
            title='Reset',
            value_type='core.type.bool',
            mandatory=False
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
            name='integral',
            title='Accumulated Integral term',
            value_type='core.type.v3f64'
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
