from ctlst import *


Function(
    name='core.cont.mux3f64',
    title=LocalizedString(
        en='Multiplexor, 2ch, 3d vector as signal'
    ),
    description=None,
    inputs=[
        Input(
            name='input0',
            title='Input 1 vector',
            value_type='core.type.vector3f64'
        ),
        Input(
            name='input1',
            title='Input 2 vector',
            value_type='core.type.vector3f64'
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
            value_type='core.type.vector3f64'
        )
    ]
)
