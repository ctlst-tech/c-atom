cmake_minimum_required(VERSION 3.6)
project(c-atom)

include_directories(./function)

##### before configuration call
##### ./c-atom/tools/fspecgen.py --code --cmake --registry_c ./f_specs_reg.c --f_specs_dirs f_specs/ f_specs2/
#####

#include(fspecs.cmake)

add_subdirectory(eswb)
add_subdirectory(fsminst/fsmlib)
add_subdirectory(xml)

set(FSMINST_SRC
        fsminst/fsminst.c
        fsminst/fsminst.h
        fsminst/fsminst_parser_xml.c
        )

add_library(fsminst-static STATIC ${FSMINST_SRC})

target_include_directories(fsminst-static PUBLIC
        eswb/src/lib/include/public
        xml
        fsminst/fsmlib
        fsminst/fsmlib/clk
        fsminst/fsmlib/eint
        )

set(CATOM_SRC
        flow/flow.c
        function/function.c
        function/error.c
        swsys/swsys.c
        swsys/swsys_services.c
        swsys/swsys.h
        swsys/swsys_parser_xml.c
        ibr/ibr.c
        ibr/ibr.h
        ibr/ibr_msg.c
        ibr/ibr_msg.h
        ibr/ibr_parser_xml.c
        ibr/ibr_process.c
        ibr/ibr_process.h
        ibr/ibr_convert.c
        ibr/ibr_convert.h
        xml/xml.c
        xml/xml.h
        function/common_parser_xml.c
        catom.c
        flow/flow_parser_xml.c
        flow/flow.h
        )

add_library(c-atom-static STATIC ${CATOM_SRC})

if(CATOM_NO_SOCKET)
    target_compile_definitions(c-atom-static PUBLIC CATOM_NO_SOCKET=1)
endif()

target_include_directories(c-atom-static PUBLIC
        f_specs
        fsminst/fsmlib
        fsminst/fsmlib/clk
        )

target_include_directories(c-atom-static PUBLIC
        eswb/src/lib/include/public
        xml
        flow
        fsminst
        fsminst/fsmlib
        ibr
        )

target_link_libraries(c-atom-static PRIVATE
        eswb-if
        fsminst-static
        fsm-static
        xml-static
        )


#set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wl,-Map=mock.map")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
