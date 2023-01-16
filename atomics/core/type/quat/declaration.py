from fspeclib import *


Structure(
    name='core.type.quat',
    title=LocalizedString(
        en='Quaternion'
    ),
    description=LocalizedString(
        en='''Quaternion that can represent a rotation about an axis in 3-D space.'''
    ),
    fields=[
        Field(
            name='w',
            title=LocalizedString(
                en='''The quaternion's W-component'''
            ),
            value_type='core.type.f64'
        ),
        Field(
            name='x',
            title=LocalizedString(
                en='''The quaternion's X-component'''
            ),
            value_type='core.type.f64'
        ),
        Field(
            name='y',
            title=LocalizedString(
                en='''The quaternion's Y-component'''
            ),
            value_type='core.type.f64'
        ),
        Field(
            name='z',
            title=LocalizedString(
                en='''The quaternion's Z-component'''
            ),
            value_type='core.type.f64'
        )
    ]
)
