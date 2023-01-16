from fspeclib import *

Function(
    name='core.source.sin',
    title=LocalizedString(
        en='Sinus with constant period'
    ),
    description=None,
    parameters=[
        Parameter(
            name='period',
            title='Sinus period',
            value_type='core.type.f64',
            constraints=[
                ThisValue() > 0
            ]
        ),
        Parameter(
            name='phase',
            title='Initial phase',
            value_type='core.type.f64',
            default=0,
        ),
        ComputedParameter(
            name='omega',
            title='Cyclic frequency',
            value_type='core.type.f64'
        )
    ],
    outputs=[
        Output(
            name='out',
            title='Sinus value',
            value_type='core.type.f64'
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
