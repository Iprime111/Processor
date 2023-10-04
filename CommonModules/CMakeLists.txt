project (CommonModules)

add_library (CommonModules src/CommonModules.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
