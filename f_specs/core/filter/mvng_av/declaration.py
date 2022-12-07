from fspeclib import *


Function(
    name='core.filter.mvng_av',
    title=LocalizedString(
        en='Moving average 3 channels function'
    ),
    has_pre_exec_init_call=True,
    inputs=[
        Input(
            name='i1',
            title='Input 1',
            value_type='core.type.f64'
        ),
        Input(
            name='i2',
            title='Input 2',
            value_type='core.type.f64'
        ),
        Input(
            name='i3',
            title='Input 3',
            value_type='core.type.f64'
        ),
    ],
    outputs=[
        Output(
            name='a1',
            title='Average 1',
            value_type='core.type.f64'
        ),
        Output(
            name='a2',
            title='Average 2',
            value_type='core.type.f64'
        ),
        Output(
            name='a3',
            title='Average 3',
            value_type='core.type.f64'
        ),
        Output(
            name='filled',
            title='Filled',
            description='Is set to TRUE when selection is initually filled',
            value_type='core.type.bool'
        ),
    ],

    state=[
        Variable(
            name='counter',
            title='',
            value_type='core.type.u32'
        ),

        # TODO allocated array of the desired type
    ],
)
