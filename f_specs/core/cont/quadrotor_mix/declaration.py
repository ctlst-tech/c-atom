from fspeclib import *


Function(
    name='core.cont.quadrotor_mix',
    title=LocalizedString(
        en='Mixer for the classic X quad-rotor control signal'
    ),
    parameters=[
        Parameter(
            name='Kt_m1',
            title='Transversal control weight for Motor 1',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kt_m2',
            title='Transversal control weight for Motor 2',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kt_m3',
            title='Transversal control weight for Motor 3',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kt_m4',
            title='Transversal control weight for Motor 4',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kl_m1',
            title='Longitudal control weight Motor 1',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kl_m2',
            title='Longitudal control weight Motor 2',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kl_m3',
            title='Longitudal control weight Motor 3',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kl_m4',
            title='Longitudal control weight Motor 4',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kc',
            title='Collective control weight for all motors',
            value_type='core.type.f64',
        ),

        Parameter(
            name='Kr_m1',
            title='Rudder control weight for Motor 1',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kr_m2',
            title='Rudder control weight for Motor 2',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kr_m3',
            title='Rudder control weight for Motor 3',
            value_type='core.type.f64',
        ),
        Parameter(
            name='Kr_m4',
            title='Rudder control weight for Motor 4',
            value_type='core.type.f64',
        ),

    ],
    inputs=[
        Input(
            name='transv',
            title='Input',
            value_type='core.type.f64'
        ),
        Input(
            name='longit',
            title='Feedback',
            value_type='core.type.f64'
        ),
        Input(
            name='rudder',
            title='Enable',
            value_type='core.type.f64',
            mandatory=False
        ),
        Input(
            name='collective',
            title='Enable',
            value_type='core.type.f64',
            mandatory=False
        ),
    ],
    outputs=[
        Output(
            name='m1',
            title='Motor 1 ESC output',
            value_type='core.type.f64'
        ),
        Output(
            name='m2',
            title='Motor 2 ESC output',
            value_type='core.type.f64'
        ),
        Output(
            name='m3',
            title='Motor 3 ESC output',
            value_type='core.type.f64'
        ),
        Output(
            name='m4',
            title='Motor 4 ESC output',
            value_type='core.type.f64'
        ),
    ],

)
