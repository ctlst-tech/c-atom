from fspeclib import *


Function(
    name='core.math.vector_calib',
    title=LocalizedString(
        en='Vector calibration'
    ),

    inputs=[
        Input(
            name='in',
            title='3d vector',
            value_type='core.type.v3f64'
        ),
    ],

    parameters=[
        Parameter(
            name='a11',
            title='a11 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=1.0,
        ),
        Parameter(
            name='a12',
            title='a12 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a13',
            title='a13 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a21',
            title='a21 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a22',
            title='a22 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=1.0,
        ),
        Parameter(
            name='a23',
            title='a23 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a31',
            title='a31 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a32',
            title='a32 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='a33',
            title='a33 calib value',
            value_type='core.type.f64',
            tunable=True,
            default=1.0,
        ),
        Parameter(
            name='b1',
            title='b1 bias calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='b2',
            title='b2 bias calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
        Parameter(
            name='b3',
            title='b3 bias calib value',
            value_type='core.type.f64',
            tunable=True,
            default=0.0,
        ),
    ],

    outputs=[
        Output(
            name='out',
            title='3d vector',
            value_type='core.type.v3f64'
        ),
    ],
)
