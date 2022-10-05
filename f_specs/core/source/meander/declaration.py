from ctlst import *

Function(
    name='core.source.meander',
    title=LocalizedString(
        en='Meander with constant period'
    ),
    description=None,
    parameters=[
        Parameter(
            name='period',
            title='Meadner period',
            value_type='core.type.f64',
            constraints=[
                ThisValue() > 0
            ]
        ),
        Parameter(
            name='duty',
            title='Meadner duty cycle',
            value_type='core.type.f64',
            constraints=[
                ThisValue() > 0
            ]
        ),
    ],
    outputs=[
        Output(
            name='out',
            title='Meander value',
            value_type='core.type.bool'
        )
    ],
    state=[
        Variable(
        name='time',
        title='Free run time accumulator',
        value_type='core.type.f64'
        )
    ],
    injection=Injection(
        timedelta=True
    )
)
