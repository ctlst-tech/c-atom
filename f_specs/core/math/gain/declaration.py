from ctlst import *


Function(
    name='core.math.gain',
    title=LocalizedString(
        en='Gain of number'
    ),
    description=LocalizedString(
        en='This function returns input value multiplied by a specified gain factor'
    ),
    parameters=[
      Parameter(
          name='gain',
          title='Gain',
          value_type='core.type.f64',
      )
    ],
    inputs=[
        Input(
            name='input',
            title='Input',
            value_type='core.type.f64'
        )
    ],
    outputs=[
        Output(
            name='output',
            title='Output',
            value_type='core.type.f64'
        )
    ]
)
