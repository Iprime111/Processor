cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

set (COMPILE_FLAGS -O3)

add_link_options (${COMPILE_FLAGS})
add_compile_options (${COMPILE_FLAGS})
add_compile_definitions (_SHOW_STACK_TRACE _NDEBUG)

option (BUILD_SHARED_LIBS "Build shared libraries" OFF)

project (processor)

add_subdirectory (libs)

add_subdirectory (CommonModules)
add_subdirectory (SoftProcessor)
add_subdirectory (Assembler)
add_subdirectory (Disassembler)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_target_properties (Assembler Disassembler SoftProcessor CommonModules FileIO CustomAssert Stack ConsoleParser ColorConsole
                       PROPERTIES
                       ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                       LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                       RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


