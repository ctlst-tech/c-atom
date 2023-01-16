from fspeclib import *


Function(
    name='core.math.lerp',
    title=LocalizedString(
        en='Linear interpolation between two numbers'
    ),
    description=None,
    inputs=[
        Input(
            name='factor',
            title='Factor',
            value_type='core.type.f64'
        ),
        Input(
            name='start',
            title='Start value',
            value_type='core.type.f64'
        ),
        Input(
            name='end',
            title='End value',
            value_type='core.type.f64'
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
