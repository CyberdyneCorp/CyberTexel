include_guard(GLOBAL)

function(ctex_configure_c_target target)
    target_compile_features(${target} PUBLIC c_std_11)
    target_include_directories(
        ${target}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated/include>
    )
    set_target_properties(${target} PROPERTIES C_EXTENSIONS OFF)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(CTEX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # A C++20 designated initializer value-initializes the members it
            # omits; that is the standard's guarantee and the idiom this
            # codebase's descriptors rely on. GCC still reports every omitted
            # member under -Wextra, which Clang and MSVC do not. The warning
            # carries no information here, so it is off for GCC only and every
            # other warning stays an error.
            target_compile_options(${target} PRIVATE -Wno-missing-field-initializers)
        endif()
        if(CTEX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(ctex_configure_cpp_target target)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_include_directories(
        ${target}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated/include>
    )

    set_target_properties(
        ${target}
        PROPERTIES
            CXX_EXTENSIONS OFF
            CXX_VISIBILITY_PRESET hidden
            POSITION_INDEPENDENT_CODE ON
            VISIBILITY_INLINES_HIDDEN YES
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        if(CTEX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # A C++20 designated initializer value-initializes the members it
            # omits; that is the standard's guarantee and the idiom this
            # codebase's descriptors rely on. GCC still reports every omitted
            # member under -Wextra, which Clang and MSVC do not. The warning
            # carries no information here, so it is off for GCC only and every
            # other warning stays an error.
            target_compile_options(${target} PRIVATE -Wno-missing-field-initializers)
        endif()
        if(CTEX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(ctex_add_module name)
    cmake_parse_arguments(MODULE "" "" "DEPENDS;SOURCES" ${ARGN})
    set(target ctex_${name})

    if(MODULE_SOURCES)
        set(sources ${MODULE_SOURCES})
    else()
        set(sources src/${name}/${name}.cpp)
    endif()
    add_library(${target} OBJECT ${sources})
    add_library(CyberTexel::${name} ALIAS ${target})
    ctex_configure_cpp_target(${target})

    foreach(dependency IN LISTS MODULE_DEPENDS)
        target_link_libraries(${target} PUBLIC ctex_${dependency})
    endforeach()

    set_property(GLOBAL APPEND PROPERTY CTEX_MODULES ${name})
endfunction()

function(ctex_add_library)
    get_property(modules GLOBAL PROPERTY CTEX_MODULES)
    set(module_objects)
    foreach(module IN LISTS modules)
        list(APPEND module_objects $<TARGET_OBJECTS:ctex_${module}>)
    endforeach()

    add_library(cybertexel STATIC ${module_objects})
    add_library(CyberTexel::cybertexel ALIAS cybertexel)
    ctex_configure_cpp_target(cybertexel)
    set_target_properties(
        cybertexel
        PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        add_library(cybertexel_c STATIC ${module_objects})
    else()
        add_library(cybertexel_c SHARED ${module_objects})
    endif()
    add_library(CyberTexel::capi_shared ALIAS cybertexel_c)
    ctex_configure_cpp_target(cybertexel_c)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        target_compile_definitions(cybertexel_c INTERFACE CTEX_SHARED)
        if(APPLE)
            target_link_options(
                cybertexel_c
                PRIVATE
                    "LINKER:-exported_symbols_list,${PROJECT_SOURCE_DIR}/cmake/exports/cybertexel.exports"
            )
            set_property(
                TARGET cybertexel_c
                APPEND PROPERTY LINK_DEPENDS "${PROJECT_SOURCE_DIR}/cmake/exports/cybertexel.exports"
            )
        elseif(WIN32)
            target_sources(
                cybertexel_c PRIVATE "${PROJECT_SOURCE_DIR}/cmake/exports/cybertexel.def"
            )
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_link_options(
                cybertexel_c
                PRIVATE
                    "LINKER:--version-script=${PROJECT_SOURCE_DIR}/cmake/exports/cybertexel.map"
            )
            set_property(
                TARGET cybertexel_c
                APPEND PROPERTY LINK_DEPENDS "${PROJECT_SOURCE_DIR}/cmake/exports/cybertexel.map"
            )
        endif()
    endif()
    set_target_properties(
        cybertexel_c
        PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
    )
endfunction()
