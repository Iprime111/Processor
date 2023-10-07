project (CommonModules)

add_library (CommonModules src/CommonModules.cpp)

target_link_libraries (CommonModules PRIVATE CustomAssert)
target_link_libraries (CommonModules PRIVATE FileIO)
target_link_libraries (CommonModules PRIVATE Stack)
target_include_directories (CommonModules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
