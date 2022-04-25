from ctlst import *


Function(
    name='core.controller.mux',
    title=LocalizedString(
        en='Multiplexor, 2ch'
    ),
    description=None,
    inputs=[
        Input(
            name='input0',
            title='Input 1',
            value_type='core.type.f64'
        ),
        Input(
            name='input1',
            title='Input 2',
            value_type='core.type.f64'
        ),
        Input(
            name='select',
            title='Switch input',
            value_type='core.type.bool'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        )
    ]
)
