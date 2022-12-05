
add_subdirectory(lib)
add_library(xml-parser-egram-static STATIC egram_xml.c parser_egram.c)
target_link_libraries(xml-parser-egram-static PUBLIC egram-static)


#add_executable(xml_egram_test )
#target_link_libraries(xml_egram_test egram-static)
#target_include_directories(xml_egram_test PUBLIC lib)
#target_compile_definitions(xml_egram_test PUBLIC XML_EGRAM_TEST=1)
