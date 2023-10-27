project (SoftProcessor)

add_executable (${PROJECT_NAME} main.cpp)

add_subdirectory (src)

find_package (SFML 2.5 COMPONENTS graphics window system REQUIRED)

target_link_libraries (${PROJECT_NAME} PRIVATE sfml-graphics)
target_link_libraries (${PROJECT_NAME} PRIVATE ColorConsole)
target_link_libraries (${PROJECT_NAME} PRIVATE CustomAssert)
target_link_libraries (${PROJECT_NAME} PRIVATE ConsoleParser)
target_link_libraries (${PROJECT_NAME} PRIVATE Stack)
target_link_libraries (${PROJECT_NAME} PRIVATE FileIO)
target_link_libraries (${PROJECT_NAME} PRIVATE readline)
target_link_libraries (${PROJECT_NAME} PRIVATE CommonModules)

target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
