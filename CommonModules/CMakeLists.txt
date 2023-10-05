project (CommonModules)

add_library (CommonModules src/CommonModules.cpp)

target_link_libraries (CommonModules PRIVATE CustomAssert)
target_include_directories (CommonModules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
