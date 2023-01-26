from fspeclib import *


Function(
    name='core.logical.rs_trigger',
    title=LocalizedString(
        en='RS-Trigger'
    ),
    description=None,
    inputs=[
        Input(
            name='r',
            title='Reset',
            value_type='core.type.bool'
        ),
        Input(
            name='s',
            title='Set',
            value_type='core.type.bool'
        )
    ],

    state=[
        Variable(
            name='state',
            title='Trigger state',
            value_type='core.type.bool'
        ),
    ],

    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.bool'
        )
    ]
)
