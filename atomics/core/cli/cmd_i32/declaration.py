from fspeclib import *

Function(
    name='core.cli.cmd_i32',
    title="Command line interface integer output",
    has_pre_exec_init_call=True,
    parameters=[
        Parameter(
            name='alias',
            title='Command alias for subscribing on',
            value_type='core.type.str',
            tunable=False,
            default=0,
        ),
        Parameter(
            name='default_output',
            title='Default output',
            value_type='core.type.i32',
            tunable=False,
            default=0,
        )
    ],

    outputs=[
        Output(
            name='output',
            title='Current bool flag value coming from CLI',
            value_type='core.type.i32',
            explicit_update=True,
        )
    ],
    state=[
        Variable(
            name='fifo_td',
            title='FIFO topic descriptor',
            value_type='core.type.td',
        ),
        Variable(
            name='value',
            title='Last value set',
            value_type='core.type.i32',
        )
    ],
    parameter_constraints=[],
    target_link_libraries=['atomics-cli-static']
)
