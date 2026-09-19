include_guard(GLOBAL)

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
            VISIBILITY_INLINES_HIDDEN YES
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        if(CTEX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
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
endfunction()
