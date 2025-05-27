cmake_minimum_required(VERSION 3.15)


add_library(atomics-cli-static STATIC atomics_cli_cmd.c)
target_include_directories(atomics-cli-static PUBLIC ./)
target_link_libraries(atomics-cli-static PUBLIC eswb-static)
