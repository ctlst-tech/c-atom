
add_subdirectory(expat)
add_library(xml-static STATIC xml.c xml.h)
target_link_libraries(xml-static PUBLIC expat-static)

add_executable(xml_test test/xml_test.c)

target_link_libraries(xml_test xml-static)

target_include_directories(xml_test PUBLIC ./)

