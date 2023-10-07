project (CommonModules)

add_library (CommonModules "")

add_subdirectory (src)

target_link_libraries (CommonModules PRIVATE ColorConsole)
target_link_libraries (CommonModules PRIVATE CustomAssert)
target_link_libraries (CommonModules PRIVATE FileIO)
target_link_libraries (CommonModules PRIVATE Stack)
target_include_directories (CommonModules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
