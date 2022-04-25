from ctlst import *


Function(
    name='core.math.rate_limiter',
    title=LocalizedString(
        en='Rate limiter'
    ),
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        ),
        Input(
            name='rate',
            title='Rate',
            description=LocalizedString(
                en='Rate limit of input value changing. Rate limit is measured in units per seconds.'
            ),
            value_type='core.type.f64'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        )
    ],
    injection=Injection(
        timedelta=True,
        timestamp=False
    )
)
