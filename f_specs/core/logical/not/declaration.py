from ctlst import *


Function(
    name='core.logical.not',
    title=LocalizedString(
        en='Logical NOT'
    ),
    description=None,
    inputs=[
        Input(
            name='input',
            title='Input',
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
