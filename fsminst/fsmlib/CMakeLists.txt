cmake_minimum_required(VERSION 3.16)

find_package(FLEX)
find_package(BISON)

if (BISON_VERSION VERSION_LESS 3.4)
    if(APPLE)
        set(BISON_EXECUTABLE "/usr/local/bin/bison")
        execute_process(COMMAND ${BISON_EXECUTABLE} --version | grep bison | awk '{print $4}'
                OUTPUT_VARIABLE BISON_VERSION)
    endif()
endif()

bison_target(parser eint/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c)
flex_target(parser eint/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.c)

set(EINT_SRC
        eint/ast.c
        eint/ast.h
        eint/symbol.c
        eint/symbol.h
        eint/instr.c
        eint/instr.h
        eint/eint.c
        eint/eint.h
        eint/eint_types.h
        ${BISON_parser_OUTPUTS}
        ${FLEX_parser_OUTPUTS}
    )

set(FSM_SRC
        fsm.h
        fsm.c
        fsm_interpr.c
        fsm_interpr.h
        clk/clk.c
        clk/clk.h
    )

set(FSM_XML_SRC
        ../fsminst_parser_xml.c
    )


add_library(fsm-static STATIC ${FSM_SRC} ${EINT_SRC})
#add_library(fsminst-static STATIC ${FSM_SRC} ${EINT_SRC} ${FSM_XML_SRC} ${FSMINST_SRC})

set(FSM_INC_DIRS
        .
        clk
        eint
        ${CMAKE_CURRENT_BINARY_DIR} # for bison generated files
        )

target_include_directories(fsm-static PUBLIC ${FSM_INC_DIRS})
#target_include_directories(fsminst-static PUBLIC ${FSM_INC_DIRS} ../atomics/include)


