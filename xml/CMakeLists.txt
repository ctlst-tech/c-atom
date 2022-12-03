find_package(Catch2 REQUIRED)

add_subdirectory(expat)
add_subdirectory(egram)

add_library(xml-static STATIC xml.c xml.h)
target_link_libraries(xml-static PUBLIC xml-parser-expat-static)

add_executable(xml_test_egram
            tests/xml_egram.cpp
            xml.c
            xml.h
            xml_priv.h
        )

target_include_directories(xml_test_egram PRIVATE src/lib/include)
target_link_libraries(xml_test_egram PUBLIC  Catch2::Catch2WithMain xml-parser-egram-static xml-parser-expat-static)


#add_executable(xml_test test/xml_test.c)
#target_link_libraries(xml_test xml-static)
#target_include_directories(xml_test PUBLIC ./)

