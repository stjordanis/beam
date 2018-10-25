set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_MODULE_PATH}")

set(BEAM_USE_GPU TRUE)
if(BEAM_POW_TYPE MATCHES "GPU")
    set(BEAM_USE_GPU TRUE)
endif()

if(BEAM_USE_GPU)
    set(POW_SRC
        equihash_gpu.cpp)
else()

    include_directories(impl)

    set(POW_SRC
        equihash.cpp
        impl/crypto/equihash_impl.cpp
        impl/arith_uint256.cpp
        impl/uint256.cpp
        impl/utilstrencodings.cpp
    )

    if(UNIX)
        set_source_files_properties(impl/crypto/equihash_impl.cpp PROPERTIES COMPILE_FLAGS -O2)
    endif()

endif()

add_library(pow STATIC ${POW_SRC})
add_dependencies(pow crypto)
target_link_libraries(pow crypto)

if(BEAM_USE_GPU)
    target_compile_definitions(pow PRIVATE BEAM_USE_GPU)
else()
    target_compile_definitions(pow PRIVATE ENABLE_MINING)
endif()

target_include_directories(pow PRIVATE ${PROJECT_SOURCE_DIR}/utility/crypto)

add_subdirectory(unittests)
