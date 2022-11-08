from fspeclib import *

Structure(
    name='core.type.vector3f64',
    title=LocalizedString(
        en='3d vector of f64 type'
    ),
    fields=[
        Field(
            name='x',
            title=LocalizedString(
                en='''The vectors's X-component'''
            ),
            value_type='core.type.f64'
        ),
        Field(
            name='y',
            title=LocalizedString(
                en='''The vectors's y-component'''
            ),
            value_type='core.type.f64'
        ),
        Field(
            name='z',
            title=LocalizedString(
                en='''The vectors's z-component'''
            ),
            value_type='core.type.f64'
        ),
    ]
)
