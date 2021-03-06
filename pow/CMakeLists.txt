set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_MODULE_PATH}")

include_directories(impl)

set(POW_SRC
    equihash.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/crypto/equihash_impl.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/arith_uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/utilstrencodings.cpp
)

if(UNIX)
    set_source_files_properties(impl/crypto/equihash_impl.cpp PROPERTIES COMPILE_FLAGS -O2)
endif()

if(BEAM_USE_GPU)

    find_package(CUDA)

    include_directories(${CUDA_INCLUDE_DIRS})

    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; --disable-warnings; --ptxas-options=-v; -use_fast_math; -lineinfo)

    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};; -gencode arch=compute_30,code=sm_30; -gencode arch=compute_35,code=sm_35; -gencode arch=compute_37,code=sm_37; -gencode arch=compute_50,code=sm_50; -gencode arch=compute_52,code=sm_52; -gencode arch=compute_60,code=sm_60; -gencode arch=compute_61,code=sm_61; -gencode arch=compute_70,code=sm_70; -gencode arch=compute_72,code=sm_72;)

    set(POW_SRC ${POW_SRC}
        equihash.cpp
        ${PROJECT_SOURCE_DIR}/3rdparty/equihash_gpu.cu)

    CUDA_ADD_LIBRARY(pow STATIC ${POW_SRC})

else()
    add_library(pow STATIC ${POW_SRC})
endif()

target_compile_definitions(pow PRIVATE ENABLE_MINING)

add_dependencies(pow crypto)
target_link_libraries(pow crypto)

target_include_directories(pow PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/crypto)

add_subdirectory(unittests)
