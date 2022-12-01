
set(EXPAT_SRC
        ascii.h
        asciitab.h
        expat.h
        catom_expat_config.h
        expat_external.h
        iasciitab.h
        internal.h
        latin1tab.h
        nametab.h
        siphash.h
        utf8tab.h
        winconfig.h
        xmlparse.c
        xmlrole.c
        xmlrole.h
        xmltok.c
        xmltok.h
        xmltok_impl.c
        xmltok_impl.h
        xmltok_ns.c
        )

add_library(expat-static STATIC ${EXPAT_SRC} )

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
