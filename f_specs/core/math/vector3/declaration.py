from fspeclib import *


Function(
    name='core.math.vector3',
    title=LocalizedString(
        en='Convert 3 scalars to 3d vector'
    ),

    inputs=[
        Input(
            name='x',
            title='X component',
            value_type='core.type.f64'
        ),
        Input(
            name='y',
            title='Y component',
            value_type='core.type.f64'
        ),
        Input(
            name='z',
            title='Z component',
            value_type='core.type.f64'
        ),
    ],

    outputs=[
        Input(
            name='v',
            title='3d vector',
            value_type='core.type.vector3f64'
        ),
    ],
)
