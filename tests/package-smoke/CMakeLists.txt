cmake_minimum_required(VERSION 3.24)

project(CyberTexelPackageSmoke LANGUAGES C CXX)

if(NOT DEFINED CYBERTEXEL_ROOT)
    message(FATAL_ERROR "CYBERTEXEL_ROOT must name an installed package tree")
endif()

find_path(
    CYBERTEXEL_INCLUDE_DIR
    NAMES ctex/capi.h ctex/version.h
    PATHS "${CYBERTEXEL_ROOT}/include"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED
)
find_library(
    CYBERTEXEL_LIBRARY
    NAMES cybertexel_c
    PATHS "${CYBERTEXEL_ROOT}/lib" "${CYBERTEXEL_ROOT}/lib64"
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
    REQUIRED
)

add_executable(cybertexel_package_smoke main.c)
target_compile_features(cybertexel_package_smoke PRIVATE c_std_11)
target_include_directories(cybertexel_package_smoke PRIVATE "${CYBERTEXEL_INCLUDE_DIR}")
target_link_libraries(cybertexel_package_smoke PRIVATE "${CYBERTEXEL_LIBRARY}")

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    foreach(dependency IN ITEMS ctex_lodepng ctex_kong tinyexr miniz)
        find_library(
            ${dependency}_LIBRARY
            NAMES ${dependency}
            PATHS "${CYBERTEXEL_ROOT}/lib" "${CYBERTEXEL_ROOT}/lib64"
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
            REQUIRED
        )
        target_link_libraries(cybertexel_package_smoke PRIVATE "${${dependency}_LIBRARY}")
    endforeach()
    set_target_properties(cybertexel_package_smoke PROPERTIES LINKER_LANGUAGE CXX)
endif()
