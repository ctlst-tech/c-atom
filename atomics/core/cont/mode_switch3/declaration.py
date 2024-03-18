from fspeclib import *


Function(
    name='core.cont.mode_switch3',
    title=LocalizedString(
        en='Discrete 3 mode switcher from proportional signal'
    ),
    description=None,
    inputs=[
        Input(
            name='input',
            title='Source signal',
            value_type='core.type.f64'
        ),
    ],
    outputs=[
        Output(
            name='mode1',
            title='Mode 1',
            value_type='core.type.bool'
        ),
        Output(
            name='mode2',
            title='Mode 2',
            value_type='core.type.bool'
        ),
        Output(
            name='mode3',
            title='Mode 3',
            value_type='core.type.bool'
        )
    ]
)
