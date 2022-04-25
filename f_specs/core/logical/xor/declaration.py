from ctlst import *


Function(
    name='core.logical.xor',
    title=LocalizedString(
        en='Logical XOR'
    ),
    description=None,
    inputs=[
        Input(
            name='input0',
            title='Input 1',
            value_type='core.type.bool'
        ),
        Input(
            name='input1',
            title='Input 2',
            value_type='core.type.bool'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.bool'
        )
    ]
)
