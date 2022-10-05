from ctlst import *

Function(
    name='core.filter.median',
    title=LocalizedString(
        en='Median filter'
    ),
    parameters=[
        Parameter(
            name='size',
            title='Selection size',
            value_type='core.type.u32',
        )
    ],
    inputs=[
        Input(
            name='input',
            title='',
            value_type='core.type.f64'
        ),
    ],
    outputs=[
        Output(
            name='output',
            title='',
            value_type='core.type.f64'
        ),
    ],

    state=[
        Variable(
            name='counter',
            title='',
            value_type='core.type.u32'
        ),
        Variable(
            name='inited',
            title='',
            value_type='core.type.bool'
        ),
        Variable(
            name='median_index',
            title='',
            value_type='core.type.u32'
        ),
        Variable(
            name='filled_in',
            title='',
            value_type='core.type.bool'
        ),
        Variable(
            name='selection',
            title='',
            value_type='core.type.dptr'
        ),
    ],
)
