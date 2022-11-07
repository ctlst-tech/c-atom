from fspeclib import *


Function(
    name='core.math.inverse_gain',
    title=LocalizedString(
        en='Gain of number'
    ),
    parameters=[
      Parameter(
          name='inverse_gain',
          title='Inverse gain',
          value_type='core.type.f64',
      ),
      ComputedParameter(
          name='gain',
          title='Gain',
          value_type='core.type.f64'
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
