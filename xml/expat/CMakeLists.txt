
add_subdirectory(lib)
add_library(xml-parser-expat-static STATIC parser_expat.c)
target_link_libraries(xml-parser-expat-static PUBLIC expat-static)
#target_include_directories(xml_test PUBLIC ./)


#add_executable(xml_test test/xml_test.c)
#target_link_libraries(xml_test xml-static)
#target_include_directories(xml_test PUBLIC ./)

