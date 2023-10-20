project (GraphicsProvider)

add_library (GraphicsProvider "")

add_subdirectory (src)

target_link_libraries (GraphicsProvider PRIVATE CommonModules)
target_link_libraries (GraphicsProvider PRIVATE ColorConsole)
target_link_libraries (GraphicsProvider PRIVATE CustomAssert)
target_link_libraries (GraphicsProvider PRIVATE FileIO)
target_link_libraries (GraphicsProvider PRIVATE Stack)
target_include_directories (GraphicsProvider PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
