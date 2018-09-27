set(TARGET_NAME wallet-jni)

add_library(${TARGET_NAME} SHARED jni.cpp)
add_dependencies(${TARGET_NAME} wallet utility)
target_link_libraries(${TARGET_NAME} wallet utility)
target_link_libraries(${TARGET_NAME} android log)

configure_file("${CMAKE_SOURCE_DIR}/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h")
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(unittests)
