cmake_minimum_required(VERSION 3.24)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CTEX_VERSION)
string(STRIP "${CTEX_VERSION}" CTEX_VERSION)
if(NOT CTEX_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a three-part semantic version")
endif()

project(CyberTexel VERSION "${CTEX_VERSION}" LANGUAGES C CXX)

find_package(Threads REQUIRED)

option(CTEX_WARNINGS_AS_ERRORS "Treat compiler warnings in CyberTexel sources as errors" ON)
option(CTEX_ENABLE_SANITIZERS "Enable address and undefined-behaviour sanitizers" OFF)
option(CTEX_BUILD_FUZZERS "Build libFuzzer targets" OFF)
option(CTEX_ENABLE_VULKAN_EXECUTOR "Build the optional owned Vulkan executor" OFF)
option(
    CTEX_BUILD_CYBER_REMESHER_EXAMPLE
    "Build the optional CyberRemesherAndUV bake-provider example"
    OFF
)

if(CTEX_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    else()
        message(FATAL_ERROR "CTEX_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
endif()

configure_file(
    cmake/version.h.in
    "${PROJECT_BINARY_DIR}/generated/include/ctex/version.h"
    @ONLY
)

include(FetchContent)
FetchContent_Declare(
    LodePNG
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    GIT_TAG ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(LodePNG)
add_library(ctex_lodepng STATIC "${lodepng_SOURCE_DIR}/lodepng.cpp")
target_include_directories(ctex_lodepng SYSTEM PUBLIC "${lodepng_SOURCE_DIR}")
target_compile_definitions(ctex_lodepng PUBLIC LODEPNG_NO_COMPILE_CPP)
set_target_properties(ctex_lodepng PROPERTIES POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(stb)

include(cmake/CyberTexelModules.cmake)

add_library(
    ctex_kong
    STATIC
        thirdparty/kong/kong.c
        thirdparty/kong/kong_cstyle.c
        thirdparty/kong/kong_hlsl.c
        thirdparty/kong/kong_metal.c
        thirdparty/kong/kong_spirv.c
        thirdparty/kong/kong_wgsl.c
        thirdparty/kong/stb_ds.c
)
target_include_directories(ctex_kong SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/thirdparty/kong")
set_target_properties(
    ctex_kong
    PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

ctex_add_module(
    image
    SOURCES
        src/image/image.cpp
        src/image/color.cpp
        src/image/color_policy.cpp
        src/image/cube_lut.cpp
        src/image/seam_dilation.cpp
)
ctex_add_module(mesh)
ctex_add_module(
    pick
    DEPENDS mesh
    SOURCES
        src/pick/pick.cpp
        src/pick/ray.cpp
        src/pick/hit.cpp
        src/pick/uv_index.cpp
        src/pick/region.cpp
        src/pick/batch.cpp
)
ctex_add_module(
    graph
    DEPENDS image
    SOURCES
        src/graph/graph.cpp
        src/graph/document.cpp
        src/graph/serialization.cpp
        src/graph/catalogue.cpp
        src/graph/material_library.cpp
        src/graph/portable_nodes.cpp
        src/graph/groups.cpp
        src/graph/validation.cpp
        src/graph/host_nodes.cpp
)
ctex_add_module(
    emit
    DEPENDS graph image
    SOURCES
        src/emit/emit.cpp
        src/emit/emission_cache.cpp
        src/emit/feature_emission.cpp
        src/emit/kong_context.cpp
        src/emit/material_emission.cpp
        src/emit/pass_plan.cpp
        src/emit/preview_emission.cpp
)
target_include_directories(ctex_emit SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/thirdparty/kong")
target_link_libraries(ctex_emit PRIVATE ctex_kong)
ctex_add_module(
    doc
    DEPENDS image mesh graph
    SOURCES
        src/doc/channels.cpp
        src/doc/document.cpp
        src/doc/material_graph.cpp
        src/doc/preset_application.cpp
        src/doc/smart_mask.cpp
        src/doc/smart_material.cpp
        src/doc/smart_material_anchors.cpp
        src/doc/smart_material_resources.cpp
)
ctex_add_module(
    paint
    DEPENDS doc emit graph pick
    SOURCES
        src/paint/paint.cpp
        src/paint/blending.cpp
        src/paint/blur_smear.cpp
        src/paint/brush.cpp
        src/paint/clone.cpp
        src/paint/colour_id.cpp
        src/paint/coverage.cpp
        src/paint/decal_stencil.cpp
        src/paint/deposition.cpp
        src/paint/fill.cpp
        src/paint/masking.cpp
        src/paint/parameters.cpp
        src/paint/particle.cpp
        src/paint/picker.cpp
        src/paint/preview.cpp
        src/paint/projection.cpp
        src/paint/rejection.cpp
        src/paint/seam_dilation.cpp
        src/paint/seam_filter.cpp
        src/paint/selection.cpp
        src/paint/surface_cache.cpp
        src/paint/stroke.cpp
        src/paint/stroke_preset.cpp
        src/paint/text.cpp
        src/paint/work.cpp
)
ctex_add_module(
    maps
    DEPENDS image mesh doc
    SOURCES
        src/maps/maps.cpp
        src/maps/mesh_maps.cpp
        src/maps/pixel_buffer_copy.cpp
        src/maps/bake_provider.cpp
        src/maps/external_import.cpp
        src/maps/generators.cpp
)
ctex_add_module(
    xport
    DEPENDS doc
    SOURCES
        src/xport/xport.cpp
        src/xport/delta.cpp
        src/xport/format.cpp
        src/xport/preview.cpp
        src/xport/readback.cpp
        src/xport/snapshot.cpp
)
ctex_add_module(
    io
    DEPENDS doc image
    SOURCES
        src/io/io.cpp
        src/io/export_preset.cpp
        src/io/export_plan.cpp
        src/io/texture_encode.cpp
        src/io/texture_export.cpp
        src/io/project_container.cpp
        src/io/project_autosave.cpp
        src/io/standalone_asset.cpp
        src/io/smart_material_package.cpp
        src/io/preset_library.cpp
)
target_include_directories(ctex_io SYSTEM PRIVATE "${stb_SOURCE_DIR}")
target_link_libraries(ctex_io PRIVATE ctex_lodepng Threads::Threads)
set(
    CTEX_EXEC_SOURCES
    src/exec/exec.cpp
    src/exec/cpu_reference.cpp
    src/exec/cpu_execution.cpp
    src/exec/execution_control.cpp
    src/exec/host_execution.cpp
    src/exec/emission_features.cpp
    src/exec/parity.cpp
    src/exec/parity_gate.cpp
)
if(CTEX_ENABLE_VULKAN_EXECUTOR)
    FetchContent_Declare(
        VulkanHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG 217e93c664ec6704ec2d8c36fa116c1a4a1e2d40
        GIT_SHALLOW FALSE
    )
    FetchContent_Declare(
        Volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG f2a16e3e19c2349b873343b2dc38a1d4c25af23a
        GIT_SHALLOW FALSE
    )
    set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
    set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(VulkanHeaders Volk)
    target_link_libraries(volk PUBLIC Vulkan::Headers)
    list(APPEND CTEX_EXEC_SOURCES src/exec/vulkan_executor.cpp)
else()
    list(APPEND CTEX_EXEC_SOURCES src/exec/vulkan_executor_stub.cpp)
endif()

ctex_add_module(exec DEPENDS emit image SOURCES ${CTEX_EXEC_SOURCES})
target_link_libraries(ctex_exec PRIVATE Threads::Threads)
if(CTEX_ENABLE_VULKAN_EXECUTOR)
    target_link_libraries(ctex_exec PRIVATE volk::volk)
endif()
ctex_add_module(capi DEPENDS image mesh pick graph emit doc paint maps xport io exec)

ctex_add_library()
target_link_libraries(cybertexel PUBLIC ctex_lodepng ctex_kong Threads::Threads)
if(TARGET cybertexel_c)
    target_link_libraries(cybertexel_c PRIVATE ctex_lodepng ctex_kong Threads::Threads)
endif()
if(CTEX_ENABLE_VULKAN_EXECUTOR)
    target_link_libraries(cybertexel PUBLIC volk::volk)
    if(TARGET cybertexel_c)
        target_link_libraries(cybertexel_c PRIVATE volk::volk)
    endif()
endif()

if(CTEX_BUILD_FUZZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "CTEX_BUILD_FUZZERS requires Clang")
    endif()
    add_executable(
        ctex_project_container_fuzz
        tests/fuzz/project_container_fuzz.cpp
    )
    ctex_configure_cpp_target(ctex_project_container_fuzz)
    target_compile_options(ctex_project_container_fuzz PRIVATE -fsanitize=fuzzer)
    target_link_options(ctex_project_container_fuzz PRIVATE -fsanitize=fuzzer)
    target_link_libraries(ctex_project_container_fuzz PRIVATE cybertexel)
endif()

# This is deliberately an example-only dependency. The library and its default
# build never discover or link CyberRemesherAndUV.
if(CTEX_BUILD_CYBER_REMESHER_EXAMPLE)
    find_package(CyberRemesher CONFIG REQUIRED)
    add_executable(
        ctex_cyber_remesher_provider_example
        examples/cyber_remesher_and_uv/provider.cpp
    )
    ctex_configure_cpp_target(ctex_cyber_remesher_provider_example)
    target_link_libraries(
        ctex_cyber_remesher_provider_example
        PRIVATE cybertexel cyber::capi
    )
endif()

include(CTest)
if(BUILD_TESTING)
    find_program(CTEX_PYTHON3_EXECUTABLE python3 REQUIRED)
    add_executable(ctex_version_test tests/version_test.c)
    target_include_directories(
        ctex_version_test
        PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
            "${PROJECT_BINARY_DIR}/generated/include"
    )
    target_link_libraries(ctex_version_test PRIVATE cybertexel)
    set_target_properties(ctex_version_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME version-consistency COMMAND ctex_version_test)

    if(TARGET cybertexel_c)
        add_library(ctex_capi_sibling STATIC tests/capi/sibling.c)
        ctex_configure_c_target(ctex_capi_sibling)
        add_executable(ctex_capi_foundation_test tests/capi/foundation_test.c)
        ctex_configure_c_target(ctex_capi_foundation_test)
        target_link_libraries(ctex_capi_foundation_test PRIVATE cybertexel_c ctex_capi_sibling)
        add_test(NAME c-abi-foundation COMMAND ctex_capi_foundation_test)
        add_executable(ctex_capi_caller_buffers_test tests/capi/caller_buffers_test.cpp)
        ctex_configure_cpp_target(ctex_capi_caller_buffers_test)
        target_include_directories(ctex_capi_caller_buffers_test PRIVATE src/capi)
        target_link_libraries(ctex_capi_caller_buffers_test PRIVATE cybertexel)
        add_test(NAME c-abi-caller-buffers COMMAND ctex_capi_caller_buffers_test)
        add_executable(ctex_capi_descriptors_test tests/capi/descriptors_test.cpp)
        ctex_configure_cpp_target(ctex_capi_descriptors_test)
        target_include_directories(ctex_capi_descriptors_test PRIVATE src/capi)
        target_link_libraries(ctex_capi_descriptors_test PRIVATE cybertexel)
        add_test(NAME c-abi-versioned-descriptors COMMAND ctex_capi_descriptors_test)
        add_executable(ctex_capi_threading_test tests/capi/threading_test.cpp)
        ctex_configure_cpp_target(ctex_capi_threading_test)
        target_link_libraries(ctex_capi_threading_test PRIVATE cybertexel Threads::Threads)
        add_test(NAME c-abi-two-document-concurrency COMMAND ctex_capi_threading_test)
        if(UNIX AND NOT APPLE)
            add_test(
                NAME c-abi-export-surface
                COMMAND
                    "${CTEX_PYTHON3_EXECUTABLE}" tests/tools/check_capi_exports.py
                    "$<TARGET_FILE:cybertexel_c>" "${CMAKE_NM}"
            )
            set_tests_properties(
                c-abi-export-surface PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
            add_test(
                NAME c-abi-compatibility
                COMMAND
                    "${CTEX_PYTHON3_EXECUTABLE}" tools/check_abi.py
                    "$<TARGET_FILE:cybertexel_c>" "${CMAKE_NM}"
            )
            set_tests_properties(
                c-abi-compatibility PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
        endif()
    endif()

    add_executable(ctex_tiled_image_test tests/image/tiled_image_test.cpp)
    ctex_configure_cpp_target(ctex_tiled_image_test)
    target_link_libraries(ctex_tiled_image_test PRIVATE ctex_image)
    add_test(NAME tiled-image COMMAND ctex_tiled_image_test)

    add_executable(ctex_color_test tests/image/color_test.cpp)
    ctex_configure_cpp_target(ctex_color_test)
    target_link_libraries(ctex_color_test PRIVATE ctex_image)
    add_test(NAME color-management COMMAND ctex_color_test)

    add_executable(
        ctex_color_management_scenarios_test
        tests/image/color_management_scenarios_test.cpp
    )
    ctex_configure_cpp_target(ctex_color_management_scenarios_test)
    target_link_libraries(ctex_color_management_scenarios_test PRIVATE ctex_image)
    add_test(NAME color-management-scenarios COMMAND ctex_color_management_scenarios_test)

    add_executable(ctex_png_io_test tests/io/png_io_test.cpp)
    ctex_configure_cpp_target(ctex_png_io_test)
    target_link_libraries(ctex_png_io_test PRIVATE cybertexel)
    add_test(NAME png-io COMMAND ctex_png_io_test)

    add_executable(ctex_project_container_test tests/io/project_container_test.cpp)
    ctex_configure_cpp_target(ctex_project_container_test)
    target_link_libraries(ctex_project_container_test PRIVATE cybertexel)
    add_test(NAME project-container COMMAND ctex_project_container_test)
    add_executable(ctex_project_autosave_test tests/io/project_autosave_test.cpp)
    ctex_configure_cpp_target(ctex_project_autosave_test)
    target_link_libraries(ctex_project_autosave_test PRIVATE cybertexel)
    add_test(NAME project-autosave COMMAND ctex_project_autosave_test)
    add_executable(ctex_standalone_asset_test tests/io/standalone_asset_test.cpp)
    ctex_configure_cpp_target(ctex_standalone_asset_test)
    target_link_libraries(ctex_standalone_asset_test PRIVATE cybertexel)
    add_test(NAME standalone-asset COMMAND ctex_standalone_asset_test)
    add_executable(ctex_smart_material_package_test tests/io/smart_material_package_test.cpp)
    ctex_configure_cpp_target(ctex_smart_material_package_test)
    target_link_libraries(ctex_smart_material_package_test PRIVATE cybertexel)
    add_test(NAME smart-material-resources COMMAND ctex_smart_material_package_test)
    add_executable(ctex_preset_library_test tests/io/preset_library_test.cpp)
    ctex_configure_cpp_target(ctex_preset_library_test)
    target_link_libraries(ctex_preset_library_test PRIVATE cybertexel)
    add_test(NAME preset-shelf-library COMMAND ctex_preset_library_test)
    add_executable(ctex_export_preset_test tests/io/export_preset_test.cpp)
    ctex_configure_cpp_target(ctex_export_preset_test)
    target_link_libraries(ctex_export_preset_test PRIVATE cybertexel)
    add_test(NAME export-presets COMMAND ctex_export_preset_test)
    add_executable(ctex_texture_encode_test tests/io/texture_encode_test.cpp)
    ctex_configure_cpp_target(ctex_texture_encode_test)
    target_link_libraries(ctex_texture_encode_test PRIVATE cybertexel)
    add_test(NAME texture-export-formats COMMAND ctex_texture_encode_test)
    add_executable(ctex_export_plan_test tests/io/export_plan_test.cpp)
    ctex_configure_cpp_target(ctex_export_plan_test)
    target_link_libraries(ctex_export_plan_test PRIVATE cybertexel)
    add_test(NAME texture-export-plan COMMAND ctex_export_plan_test)
    add_executable(ctex_texture_export_test tests/io/texture_export_test.cpp)
    ctex_configure_cpp_target(ctex_texture_export_test)
    target_link_libraries(ctex_texture_export_test PRIVATE cybertexel)
    add_test(NAME texture-export-execution COMMAND ctex_texture_export_test)
    set(CTEX_TEXTURE_EXPORT_REPORT "${CMAKE_CURRENT_BINARY_DIR}/texture-export-report.json")
    add_test(
        NAME texture-export-report-generate
        COMMAND ctex_texture_export_test --write-report "${CTEX_TEXTURE_EXPORT_REPORT}"
    )
    add_test(
        NAME texture-export-report-json
        COMMAND
            "${CTEX_PYTHON3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tests/tools/check_texture_export_report.py"
            "${CTEX_TEXTURE_EXPORT_REPORT}"
    )
    set_tests_properties(texture-export-report-json PROPERTIES DEPENDS texture-export-report-generate)
    add_test(
        NAME texture-export-determinism
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tools/check_determinism.py texture-export
    )
    set_tests_properties(
        texture-export-determinism PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    add_test(
        NAME io-scenario-matrix
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tests/tools/check_io_scenario_matrix.py
    )
    set_tests_properties(io-scenario-matrix PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    add_test(
        NAME project-container-determinism
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tools/check_determinism.py project-save
    )
    set_tests_properties(
        project-container-determinism PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_executable(ctex_channels_test tests/doc/channels_test.cpp)
    ctex_configure_cpp_target(ctex_channels_test)
    target_link_libraries(ctex_channels_test PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image)
    add_test(NAME document-channels COMMAND ctex_channels_test)
    add_executable(ctex_smart_material_test tests/doc/smart_material_test.cpp)
    ctex_configure_cpp_target(ctex_smart_material_test)
    target_link_libraries(
        ctex_smart_material_test PRIVATE ctex_doc ctex_graph ctex_image ctex_mesh
    )
    add_test(NAME smart-material-serialization COMMAND ctex_smart_material_test)
    add_executable(ctex_smart_mask_test tests/doc/smart_mask_test.cpp)
    ctex_configure_cpp_target(ctex_smart_mask_test)
    target_link_libraries(ctex_smart_mask_test PRIVATE ctex_doc ctex_graph ctex_image ctex_mesh)
    add_test(NAME smart-mask-instances COMMAND ctex_smart_mask_test)
    add_test(
        NAME smart-material-scenario-matrix
        COMMAND
            "${CTEX_PYTHON3_EXECUTABLE}"
            tests/tools/check_smart_material_scenario_matrix.py
    )
    set_tests_properties(
        smart-material-scenario-matrix PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_executable(ctex_host_transport_revisions_test tests/doc/revisions_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_revisions_test)
    target_link_libraries(
        ctex_host_transport_revisions_test PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-revisions COMMAND ctex_host_transport_revisions_test)

    add_executable(ctex_host_transport_delta_test tests/xport/delta_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_delta_test)
    target_link_libraries(
        ctex_host_transport_delta_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-delta COMMAND ctex_host_transport_delta_test)

    add_executable(ctex_host_transport_readback_test tests/xport/readback_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_readback_test)
    target_link_libraries(
        ctex_host_transport_readback_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-readback COMMAND ctex_host_transport_readback_test)

    add_executable(ctex_host_transport_layout_test tests/xport/layout_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_layout_test)
    target_link_libraries(
        ctex_host_transport_layout_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-layout COMMAND ctex_host_transport_layout_test)

    add_executable(ctex_host_transport_format_test tests/xport/format_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_format_test)
    target_link_libraries(
        ctex_host_transport_format_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-format COMMAND ctex_host_transport_format_test)

    add_executable(ctex_host_transport_snapshot_test tests/xport/snapshot_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_snapshot_test)
    target_link_libraries(
        ctex_host_transport_snapshot_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-snapshot COMMAND ctex_host_transport_snapshot_test)

    add_executable(ctex_host_transport_preview_test tests/xport/preview_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_preview_test)
    target_link_libraries(
        ctex_host_transport_preview_test
        PRIVATE ctex_xport ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME host-transport-preview COMMAND ctex_host_transport_preview_test)

    add_executable(ctex_host_transport_identity_test tests/xport/identity_test.cpp)
    ctex_configure_cpp_target(ctex_host_transport_identity_test)
    target_link_libraries(ctex_host_transport_identity_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME host-transport-identity COMMAND ctex_host_transport_identity_test)

    add_executable(ctex_stroke_reconstruction_test tests/paint/stroke_reconstruction_test.cpp)
    ctex_configure_cpp_target(ctex_stroke_reconstruction_test)
    target_link_libraries(
        ctex_stroke_reconstruction_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME stroke-reconstruction COMMAND ctex_stroke_reconstruction_test)

    add_executable(ctex_stroke_preset_test tests/paint/stroke_preset_test.cpp)
    ctex_configure_cpp_target(ctex_stroke_preset_test)
    target_link_libraries(
        ctex_stroke_preset_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME stroke-preset COMMAND ctex_stroke_preset_test)

    add_executable(ctex_paint_coverage_test tests/paint/coverage_test.cpp)
    ctex_configure_cpp_target(ctex_paint_coverage_test)
    target_link_libraries(
        ctex_paint_coverage_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-coverage COMMAND ctex_paint_coverage_test)

    add_executable(ctex_paint_decal_stencil_test tests/paint/decal_stencil_test.cpp)
    ctex_configure_cpp_target(ctex_paint_decal_stencil_test)
    target_link_libraries(
        ctex_paint_decal_stencil_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-decal-stencil COMMAND ctex_paint_decal_stencil_test)

    add_executable(ctex_paint_projection_test tests/paint/projection_test.cpp)
    ctex_configure_cpp_target(ctex_paint_projection_test)
    target_link_libraries(
        ctex_paint_projection_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-projection COMMAND ctex_paint_projection_test)

    add_executable(ctex_paint_text_test tests/paint/text_test.cpp)
    ctex_configure_cpp_target(ctex_paint_text_test)
    target_link_libraries(
        ctex_paint_text_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-text COMMAND ctex_paint_text_test)

    add_executable(ctex_paint_particle_test tests/paint/particle_test.cpp)
    ctex_configure_cpp_target(ctex_paint_particle_test)
    target_link_libraries(
        ctex_paint_particle_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-particle COMMAND ctex_paint_particle_test)

    add_executable(ctex_paint_picker_test tests/paint/picker_test.cpp)
    ctex_configure_cpp_target(ctex_paint_picker_test)
    target_link_libraries(
        ctex_paint_picker_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-picker COMMAND ctex_paint_picker_test)

    add_executable(ctex_paint_colour_id_test tests/paint/colour_id_test.cpp)
    ctex_configure_cpp_target(ctex_paint_colour_id_test)
    target_link_libraries(
        ctex_paint_colour_id_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-colour-id COMMAND ctex_paint_colour_id_test)

    add_executable(ctex_paint_selection_test tests/paint/selection_test.cpp)
    ctex_configure_cpp_target(ctex_paint_selection_test)
    target_link_libraries(
        ctex_paint_selection_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-selection COMMAND ctex_paint_selection_test)

    add_executable(ctex_paint_parameters_test tests/paint/parameters_test.cpp)
    ctex_configure_cpp_target(ctex_paint_parameters_test)
    target_link_libraries(
        ctex_paint_parameters_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-parameters COMMAND ctex_paint_parameters_test)

    add_executable(ctex_paint_rejection_test tests/paint/rejection_test.cpp)
    ctex_configure_cpp_target(ctex_paint_rejection_test)
    target_link_libraries(
        ctex_paint_rejection_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-rejection COMMAND ctex_paint_rejection_test)

    add_executable(ctex_paint_deposition_test tests/paint/deposition_test.cpp)
    ctex_configure_cpp_target(ctex_paint_deposition_test)
    target_link_libraries(
        ctex_paint_deposition_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-deposition COMMAND ctex_paint_deposition_test)

    add_executable(ctex_paint_blending_test tests/paint/blending_test.cpp)
    ctex_configure_cpp_target(ctex_paint_blending_test)
    target_link_libraries(
        ctex_paint_blending_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-blending COMMAND ctex_paint_blending_test)

    add_executable(ctex_paint_blur_smear_test tests/paint/blur_smear_test.cpp)
    ctex_configure_cpp_target(ctex_paint_blur_smear_test)
    target_link_libraries(
        ctex_paint_blur_smear_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-blur-smear COMMAND ctex_paint_blur_smear_test)

    add_executable(ctex_paint_brush_test tests/paint/brush_test.cpp)
    ctex_configure_cpp_target(ctex_paint_brush_test)
    target_link_libraries(
        ctex_paint_brush_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-brush-eraser COMMAND ctex_paint_brush_test)

    add_executable(ctex_paint_clone_test tests/paint/clone_test.cpp)
    ctex_configure_cpp_target(ctex_paint_clone_test)
    target_link_libraries(
        ctex_paint_clone_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-clone COMMAND ctex_paint_clone_test)

    add_executable(ctex_paint_fill_test tests/paint/fill_test.cpp)
    ctex_configure_cpp_target(ctex_paint_fill_test)
    target_link_libraries(
        ctex_paint_fill_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-fill COMMAND ctex_paint_fill_test)

    add_executable(ctex_paint_masking_test tests/paint/masking_test.cpp)
    ctex_configure_cpp_target(ctex_paint_masking_test)
    target_link_libraries(
        ctex_paint_masking_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-masking COMMAND ctex_paint_masking_test)

    add_executable(ctex_paint_surface_cache_test tests/paint/surface_cache_test.cpp)
    ctex_configure_cpp_target(ctex_paint_surface_cache_test)
    target_link_libraries(
        ctex_paint_surface_cache_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-surface-cache COMMAND ctex_paint_surface_cache_test)

    add_executable(ctex_paint_seam_dilation_test tests/paint/seam_dilation_test.cpp)
    ctex_configure_cpp_target(ctex_paint_seam_dilation_test)
    target_link_libraries(
        ctex_paint_seam_dilation_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-seam-dilation COMMAND ctex_paint_seam_dilation_test)

    add_executable(ctex_paint_seam_filter_test tests/paint/seam_filter_test.cpp)
    ctex_configure_cpp_target(ctex_paint_seam_filter_test)
    target_link_libraries(
        ctex_paint_seam_filter_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-seam-filter COMMAND ctex_paint_seam_filter_test)

    add_executable(ctex_paint_preview_test tests/paint/preview_test.cpp)
    ctex_configure_cpp_target(ctex_paint_preview_test)
    target_link_libraries(
        ctex_paint_preview_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-preview COMMAND ctex_paint_preview_test)

    add_executable(ctex_paint_work_test tests/paint/work_test.cpp)
    ctex_configure_cpp_target(ctex_paint_work_test)
    target_link_libraries(
        ctex_paint_work_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-work COMMAND ctex_paint_work_test)

    add_executable(ctex_paint_scenarios_test tests/paint/scenarios_test.cpp)
    ctex_configure_cpp_target(ctex_paint_scenarios_test)
    target_link_libraries(
        ctex_paint_scenarios_test
        PRIVATE ctex_paint ctex_doc ctex_emit ctex_pick ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME paint-scenarios COMMAND ctex_paint_scenarios_test)

    add_executable(ctex_mesh_maps_test tests/maps/mesh_maps_test.cpp)
    ctex_configure_cpp_target(ctex_mesh_maps_test)
    target_link_libraries(
        ctex_mesh_maps_test PRIVATE ctex_maps ctex_doc ctex_image ctex_mesh ctex_graph
    )
    add_test(NAME mesh-maps COMMAND ctex_mesh_maps_test)

    add_executable(ctex_bake_provider_test tests/maps/bake_provider_test.cpp)
    ctex_configure_cpp_target(ctex_bake_provider_test)
    target_link_libraries(
        ctex_bake_provider_test PRIVATE ctex_maps ctex_doc ctex_image ctex_mesh ctex_graph
    )
    add_test(NAME mesh-map-bake-provider COMMAND ctex_bake_provider_test)

    add_executable(ctex_external_map_import_test tests/maps/external_import_test.cpp)
    ctex_configure_cpp_target(ctex_external_map_import_test)
    target_link_libraries(
        ctex_external_map_import_test PRIVATE ctex_maps ctex_doc ctex_image ctex_mesh ctex_graph
    )
    add_test(NAME mesh-map-external-import COMMAND ctex_external_map_import_test)

    add_executable(ctex_mesh_map_generators_test tests/maps/generators_test.cpp)
    ctex_configure_cpp_target(ctex_mesh_map_generators_test)
    target_link_libraries(
        ctex_mesh_map_generators_test PRIVATE ctex_maps ctex_doc ctex_image ctex_mesh ctex_graph
    )
    add_test(NAME mesh-map-generators COMMAND ctex_mesh_map_generators_test)

    add_executable(ctex_mesh_map_generator_parity_test tests/maps/generator_parity_test.cpp)
    ctex_configure_cpp_target(ctex_mesh_map_generator_parity_test)
    target_link_libraries(
        ctex_mesh_map_generator_parity_test
        PRIVATE ctex_maps ctex_exec ctex_doc ctex_emit ctex_image ctex_mesh ctex_graph
    )
    add_test(NAME mesh-map-generator-parity COMMAND ctex_mesh_map_generator_parity_test)

    add_executable(ctex_document_test tests/doc/document_test.cpp)
    ctex_configure_cpp_target(ctex_document_test)
    target_link_libraries(ctex_document_test PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image)
    add_test(NAME texture-document COMMAND ctex_document_test)

    add_executable(ctex_mesh_test tests/mesh/mesh_test.cpp)
    ctex_configure_cpp_target(ctex_mesh_test)
    target_link_libraries(ctex_mesh_test PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image)
    add_test(NAME mesh-ingest COMMAND ctex_mesh_test)

    add_executable(ctex_spatial_index_test tests/pick/spatial_index_test.cpp)
    ctex_configure_cpp_target(ctex_spatial_index_test)
    target_link_libraries(ctex_spatial_index_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-spatial-index COMMAND ctex_spatial_index_test)

    add_executable(ctex_ray_construction_test tests/pick/ray_construction_test.cpp)
    ctex_configure_cpp_target(ctex_ray_construction_test)
    target_link_libraries(ctex_ray_construction_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-ray-construction COMMAND ctex_ray_construction_test)

    add_executable(ctex_hit_record_test tests/pick/hit_record_test.cpp)
    ctex_configure_cpp_target(ctex_hit_record_test)
    target_link_libraries(ctex_hit_record_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-hit-record COMMAND ctex_hit_record_test)

    add_executable(ctex_occlusion_test tests/pick/occlusion_test.cpp)
    ctex_configure_cpp_target(ctex_occlusion_test)
    target_link_libraries(ctex_occlusion_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-occlusion COMMAND ctex_occlusion_test)

    add_executable(ctex_uv_picking_test tests/pick/uv_picking_test.cpp)
    ctex_configure_cpp_target(ctex_uv_picking_test)
    target_link_libraries(ctex_uv_picking_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-uv-space COMMAND ctex_uv_picking_test)

    add_executable(ctex_surface_snap_test tests/pick/surface_snap_test.cpp)
    ctex_configure_cpp_target(ctex_surface_snap_test)
    target_link_libraries(ctex_surface_snap_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-surface-snap COMMAND ctex_surface_snap_test)

    add_executable(ctex_region_query_test tests/pick/region_query_test.cpp)
    ctex_configure_cpp_target(ctex_region_query_test)
    target_link_libraries(ctex_region_query_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-region-queries COMMAND ctex_region_query_test)

    add_executable(ctex_boundary_determinism_test tests/pick/boundary_determinism_test.cpp)
    ctex_configure_cpp_target(ctex_boundary_determinism_test)
    target_link_libraries(ctex_boundary_determinism_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-boundary-determinism COMMAND ctex_boundary_determinism_test)

    add_executable(ctex_batch_pick_test tests/pick/batch_pick_test.cpp)
    ctex_configure_cpp_target(ctex_batch_pick_test)
    target_link_libraries(ctex_batch_pick_test PRIVATE ctex_pick ctex_mesh)
    add_test(NAME picking-batch COMMAND ctex_batch_pick_test)

    set_tests_properties(
        picking-spatial-index
        picking-ray-construction
        picking-hit-record
        picking-occlusion
        picking-uv-space
        picking-surface-snap
        picking-region-queries
        picking-boundary-determinism
        picking-batch
        PROPERTIES LABELS picking-scenario
    )

    add_executable(ctex_graph_document_test tests/graph/document_test.cpp)
    ctex_configure_cpp_target(ctex_graph_document_test)
    target_link_libraries(ctex_graph_document_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-document COMMAND ctex_graph_document_test)

    add_executable(ctex_graph_cycle_test tests/graph/cycle_detection_test.cpp)
    ctex_configure_cpp_target(ctex_graph_cycle_test)
    target_link_libraries(ctex_graph_cycle_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-cycle-detection COMMAND ctex_graph_cycle_test)

    add_executable(ctex_graph_socket_test tests/graph/socket_links_test.cpp)
    ctex_configure_cpp_target(ctex_graph_socket_test)
    target_link_libraries(ctex_graph_socket_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-socket-links COMMAND ctex_graph_socket_test)

    add_executable(ctex_graph_catalogue_test tests/graph/catalogue_test.cpp)
    ctex_configure_cpp_target(ctex_graph_catalogue_test)
    target_link_libraries(ctex_graph_catalogue_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-catalogue COMMAND ctex_graph_catalogue_test)

    add_executable(ctex_graph_groups_test tests/graph/groups_test.cpp)
    ctex_configure_cpp_target(ctex_graph_groups_test)
    target_link_libraries(ctex_graph_groups_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-groups COMMAND ctex_graph_groups_test)

    add_executable(ctex_graph_validation_test tests/graph/validation_test.cpp)
    ctex_configure_cpp_target(ctex_graph_validation_test)
    target_link_libraries(ctex_graph_validation_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-validation COMMAND ctex_graph_validation_test)

    add_executable(ctex_graph_host_nodes_test tests/graph/host_nodes_test.cpp)
    ctex_configure_cpp_target(ctex_graph_host_nodes_test)
    target_link_libraries(ctex_graph_host_nodes_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-host-nodes COMMAND ctex_graph_host_nodes_test)

    add_executable(ctex_material_library_test tests/graph/material_library_test.cpp)
    ctex_configure_cpp_target(ctex_material_library_test)
    target_link_libraries(ctex_material_library_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-library COMMAND ctex_material_library_test)

    add_executable(ctex_portable_nodes_test tests/graph/portable_nodes_test.cpp)
    ctex_configure_cpp_target(ctex_portable_nodes_test)
    target_link_libraries(ctex_portable_nodes_test PRIVATE ctex_graph ctex_image)
    add_test(NAME material-graph-portable-nodes COMMAND ctex_portable_nodes_test)

    add_executable(ctex_kong_context_test tests/emit/kong_context_test.cpp)
    ctex_configure_cpp_target(ctex_kong_context_test)
    target_link_libraries(ctex_kong_context_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME kong-context COMMAND ctex_kong_context_test)
    find_program(CTEX_SPIRV_VAL_EXECUTABLE spirv-val)
    if(CTEX_SPIRV_VAL_EXECUTABLE)
        set(CTEX_KONG_VERTEX_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/kong-vertex.spv")
        set(CTEX_KONG_FRAGMENT_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/kong-fragment.spv")
        add_test(
            NAME kong-spirv-generate
            COMMAND
                ctex_kong_context_test --write-spirv
                "${CTEX_KONG_VERTEX_SPIRV}" "${CTEX_KONG_FRAGMENT_SPIRV}"
        )
        set_tests_properties(kong-spirv-generate PROPERTIES FIXTURES_SETUP kong-spirv)
        add_test(
            NAME kong-spirv-vertex-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_KONG_VERTEX_SPIRV}"
        )
        set_tests_properties(
            kong-spirv-vertex-validation PROPERTIES FIXTURES_REQUIRED kong-spirv
        )
        add_test(
            NAME kong-spirv-fragment-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_KONG_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            kong-spirv-fragment-validation PROPERTIES FIXTURES_REQUIRED kong-spirv
        )
    endif()

    add_executable(ctex_graph_emission_test tests/emit/graph_emission_test.cpp)
    ctex_configure_cpp_target(ctex_graph_emission_test)
    target_link_libraries(ctex_graph_emission_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME graph-emission COMMAND ctex_graph_emission_test)

    add_executable(ctex_pass_plan_test tests/emit/pass_plan_test.cpp)
    ctex_configure_cpp_target(ctex_pass_plan_test)
    target_link_libraries(ctex_pass_plan_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME pass-plan COMMAND ctex_pass_plan_test)

    add_executable(ctex_feature_emission_test tests/emit/feature_emission_test.cpp)
    ctex_configure_cpp_target(ctex_feature_emission_test)
    target_link_libraries(ctex_feature_emission_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME feature-emission COMMAND ctex_feature_emission_test)
    if(CTEX_SPIRV_VAL_EXECUTABLE)
        set(CTEX_LAYER_STACK_VERTEX_SPIRV
            "${CMAKE_CURRENT_BINARY_DIR}/layer-stack-vertex.spv"
        )
        set(CTEX_LAYER_STACK_FRAGMENT_SPIRV
            "${CMAKE_CURRENT_BINARY_DIR}/layer-stack-fragment.spv"
        )
        add_test(
            NAME feature-emission-spirv-generate
            COMMAND
                ctex_feature_emission_test --write-spirv
                "${CTEX_LAYER_STACK_VERTEX_SPIRV}" "${CTEX_LAYER_STACK_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            feature-emission-spirv-generate PROPERTIES FIXTURES_SETUP feature-emission-spirv
        )
        add_test(
            NAME feature-emission-spirv-vertex-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_LAYER_STACK_VERTEX_SPIRV}"
        )
        set_tests_properties(
            feature-emission-spirv-vertex-validation
            PROPERTIES FIXTURES_REQUIRED feature-emission-spirv
        )
        add_test(
            NAME feature-emission-spirv-fragment-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_LAYER_STACK_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            feature-emission-spirv-fragment-validation
            PROPERTIES FIXTURES_REQUIRED feature-emission-spirv
        )
    endif()

    add_executable(ctex_emission_cache_test tests/emit/emission_cache_test.cpp)
    ctex_configure_cpp_target(ctex_emission_cache_test)
    target_link_libraries(ctex_emission_cache_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME emission-cache COMMAND ctex_emission_cache_test)

    add_executable(ctex_concurrent_emission_test tests/emit/concurrent_emission_test.cpp)
    ctex_configure_cpp_target(ctex_concurrent_emission_test)
    target_link_libraries(ctex_concurrent_emission_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME concurrent-emission COMMAND ctex_concurrent_emission_test)

    add_executable(ctex_preview_emission_test tests/emit/preview_emission_test.cpp)
    ctex_configure_cpp_target(ctex_preview_emission_test)
    target_link_libraries(ctex_preview_emission_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME preview-emission COMMAND ctex_preview_emission_test)

    add_executable(ctex_material_emission_test tests/emit/material_emission_test.cpp)
    ctex_configure_cpp_target(ctex_material_emission_test)
    target_link_libraries(ctex_material_emission_test PRIVATE ctex_emit ctex_graph)
    add_test(NAME material-emission COMMAND ctex_material_emission_test)
    if(CTEX_SPIRV_VAL_EXECUTABLE)
        set(CTEX_MATERIAL_VERTEX_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/material-vertex.spv")
        set(CTEX_MATERIAL_FRAGMENT_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/material-fragment.spv")
        add_test(
            NAME material-emission-spirv-generate
            COMMAND
                ctex_material_emission_test --write-spirv
                "${CTEX_MATERIAL_VERTEX_SPIRV}" "${CTEX_MATERIAL_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            material-emission-spirv-generate PROPERTIES FIXTURES_SETUP material-emission-spirv
        )
        add_test(
            NAME material-emission-spirv-vertex-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_MATERIAL_VERTEX_SPIRV}"
        )
        set_tests_properties(
            material-emission-spirv-vertex-validation
            PROPERTIES FIXTURES_REQUIRED material-emission-spirv
        )
        add_test(
            NAME material-emission-spirv-fragment-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_MATERIAL_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            material-emission-spirv-fragment-validation
            PROPERTIES FIXTURES_REQUIRED material-emission-spirv
        )
    endif()
    if(CTEX_SPIRV_VAL_EXECUTABLE)
        set(CTEX_PREVIEW_VERTEX_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/preview-vertex.spv")
        set(CTEX_PREVIEW_FRAGMENT_SPIRV "${CMAKE_CURRENT_BINARY_DIR}/preview-fragment.spv")
        add_test(
            NAME preview-emission-spirv-generate
            COMMAND
                ctex_preview_emission_test --write-spirv
                "${CTEX_PREVIEW_VERTEX_SPIRV}" "${CTEX_PREVIEW_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            preview-emission-spirv-generate PROPERTIES FIXTURES_SETUP preview-emission-spirv
        )
        add_test(
            NAME preview-emission-spirv-vertex-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_PREVIEW_VERTEX_SPIRV}"
        )
        set_tests_properties(
            preview-emission-spirv-vertex-validation
            PROPERTIES FIXTURES_REQUIRED preview-emission-spirv
        )
        add_test(
            NAME preview-emission-spirv-fragment-validation
            COMMAND "${CTEX_SPIRV_VAL_EXECUTABLE}" "${CTEX_PREVIEW_FRAGMENT_SPIRV}"
        )
        set_tests_properties(
            preview-emission-spirv-fragment-validation
            PROPERTIES FIXTURES_REQUIRED preview-emission-spirv
        )
    endif()

    add_executable(ctex_material_graph_output_test tests/doc/material_graph_test.cpp)
    ctex_configure_cpp_target(ctex_material_graph_output_test)
    target_link_libraries(
        ctex_material_graph_output_test
        PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME material-graph-output COMMAND ctex_material_graph_output_test)

    add_executable(ctex_executor_registry_test tests/exec/executor_registry_test.cpp)
    ctex_configure_cpp_target(ctex_executor_registry_test)
    target_link_libraries(
        ctex_executor_registry_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image
    )
    add_test(NAME executor-registry COMMAND ctex_executor_registry_test)
    add_test(
        NAME executor-environment-pin
        COMMAND ctex_executor_registry_test --process-environment
    )
    set_tests_properties(executor-environment-pin PROPERTIES ENVIRONMENT "CTEX_EXECUTOR=cpu")

    add_executable(ctex_cpu_reference_test tests/exec/cpu_reference_test.cpp)
    ctex_configure_cpp_target(ctex_cpu_reference_test)
    target_link_libraries(ctex_cpu_reference_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image)
    add_test(NAME executor-cpu-reference COMMAND ctex_cpu_reference_test)

    add_executable(ctex_execution_control_test tests/exec/execution_control_test.cpp)
    ctex_configure_cpp_target(ctex_execution_control_test)
    target_link_libraries(
        ctex_execution_control_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image
    )
    add_test(NAME executor-bounded-execution COMMAND ctex_execution_control_test)

    add_executable(ctex_vulkan_executor_test tests/exec/vulkan_executor_test.cpp)
    ctex_configure_cpp_target(ctex_vulkan_executor_test)
    target_link_libraries(ctex_vulkan_executor_test PRIVATE cybertexel)
    add_test(NAME executor-vulkan COMMAND ctex_vulkan_executor_test)

    add_executable(ctex_host_execution_test tests/exec/host_execution_test.cpp)
    ctex_configure_cpp_target(ctex_host_execution_test)
    target_link_libraries(ctex_host_execution_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image)
    add_test(NAME executor-host-execution COMMAND ctex_host_execution_test)

    add_executable(ctex_executor_emission_features_test tests/exec/emission_features_test.cpp)
    ctex_configure_cpp_target(ctex_executor_emission_features_test)
    target_link_libraries(
        ctex_executor_emission_features_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image
    )
    add_test(NAME executor-emission-features COMMAND ctex_executor_emission_features_test)

    add_executable(ctex_executor_parity_test tests/exec/parity_test.cpp)
    ctex_configure_cpp_target(ctex_executor_parity_test)
    target_link_libraries(ctex_executor_parity_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image)
    add_test(NAME executor-parity-tolerances COMMAND ctex_executor_parity_test)

    add_executable(ctex_executor_parity_gate_test tests/exec/parity_gate_test.cpp)
    ctex_configure_cpp_target(ctex_executor_parity_gate_test)
    target_link_libraries(
        ctex_executor_parity_gate_test PRIVATE ctex_exec ctex_emit ctex_graph ctex_image
    )
    add_test(NAME executor-parity-gate COMMAND ctex_executor_parity_gate_test)

    add_executable(ctex_executor_parity_gate tests/exec/parity_gate_main.cpp)
    ctex_configure_cpp_target(ctex_executor_parity_gate)
    target_link_libraries(
        ctex_executor_parity_gate PRIVATE ctex_exec ctex_emit ctex_graph ctex_image
    )
    add_test(NAME executor-parity-corpus COMMAND ctex_executor_parity_gate)

    add_test(
        NAME execution-backends-layering
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tests/tools/test_check_layering.py
    )
    set_tests_properties(
        execution-backends-layering PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    add_test(
        NAME shader-emission-determinism
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tools/check_determinism.py shader-emission
    )
    set_tests_properties(
        shader-emission-determinism PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    add_test(
        NAME shader-emission-licence-audit
        COMMAND "${CTEX_PYTHON3_EXECUTABLE}" tools/check_licenses.py
    )
    set_tests_properties(
        shader-emission-licence-audit PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    add_test(
        NAME mesh-map-no-baker-dependency
        COMMAND
            "${CTEX_PYTHON3_EXECUTABLE}" tools/check_no_sibling_links.py
            "${PROJECT_BINARY_DIR}"
    )
    set_tests_properties(
        mesh-map-no-baker-dependency PROPERTIES WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    set_tests_properties(
        material-graph-document
        material-graph-output
        material-graph-cycle-detection
        material-graph-socket-links
        material-graph-catalogue
        material-graph-groups
        material-graph-validation
        material-graph-host-nodes
        material-graph-library
        material-graph-portable-nodes
        PROPERTIES LABELS material-graph-scenario
    )
    set_tests_properties(
        material-graph-validation
        PROPERTIES LABELS "material-graph-scenario;mesh-maps-scenario"
    )
    set_tests_properties(
        graph-emission
        PROPERTIES LABELS "material-graph-scenario;shader-emission-scenario"
    )
    set(
        CTEX_SHADER_SCENARIO_TESTS
        kong-context
        pass-plan
        feature-emission
        emission-cache
        concurrent-emission
        preview-emission
        material-emission
        shader-emission-determinism
        shader-emission-licence-audit
    )
    if(CTEX_SPIRV_VAL_EXECUTABLE)
        list(
            APPEND CTEX_SHADER_SCENARIO_TESTS
            kong-spirv-generate
            kong-spirv-vertex-validation
            kong-spirv-fragment-validation
            feature-emission-spirv-generate
            feature-emission-spirv-vertex-validation
            feature-emission-spirv-fragment-validation
            preview-emission-spirv-generate
            preview-emission-spirv-vertex-validation
            preview-emission-spirv-fragment-validation
            material-emission-spirv-generate
            material-emission-spirv-vertex-validation
            material-emission-spirv-fragment-validation
        )
    endif()
    set_tests_properties(
        ${CTEX_SHADER_SCENARIO_TESTS} PROPERTIES LABELS shader-emission-scenario
    )
    set(
        CTEX_EXECUTION_BACKEND_SCENARIO_TESTS
        executor-registry
        executor-environment-pin
        executor-cpu-reference
        executor-bounded-execution
        executor-vulkan
        executor-host-execution
        executor-emission-features
        executor-parity-tolerances
        executor-parity-gate
        executor-parity-corpus
        execution-backends-layering
    )
    set_tests_properties(
        ${CTEX_EXECUTION_BACKEND_SCENARIO_TESTS}
        PROPERTIES LABELS execution-backends-scenario
    )
    set(
        CTEX_HOST_TRANSPORT_SCENARIO_TESTS
        host-transport-revisions
        host-transport-delta
        host-transport-readback
        host-transport-layout
        host-transport-format
        host-transport-snapshot
        host-transport-preview
        host-transport-identity
    )
    set_tests_properties(
        ${CTEX_HOST_TRANSPORT_SCENARIO_TESTS} PROPERTIES LABELS host-transport-scenario
    )
    set_tests_properties(
        stroke-reconstruction
        stroke-preset
        PROPERTIES LABELS stroke-model-scenario
    )
    set_tests_properties(
        paint-coverage
        paint-rejection
        paint-deposition
        paint-blending
        paint-masking
        paint-surface-cache
        paint-seam-dilation
        paint-seam-filter
        paint-work
        PROPERTIES LABELS paint-engine-scenario
    )
    set_tests_properties(
        paint-preview
        paint-scenarios
        PROPERTIES LABELS "stroke-model-scenario;paint-engine-scenario"
    )
    set_tests_properties(
        mesh-maps
        mesh-map-bake-provider
        mesh-map-external-import
        mesh-map-generators
        mesh-map-generator-parity
        mesh-map-no-baker-dependency
        PROPERTIES LABELS mesh-maps-scenario
    )
    set_tests_properties(mesh-ingest PROPERTIES LABELS mesh-maps-scenario)
    set_tests_properties(
        project-container
        project-autosave
        standalone-asset
        export-presets
        texture-export-formats
        texture-export-plan
        texture-export-execution
        texture-export-report-generate
        texture-export-report-json
        texture-export-determinism
        io-scenario-matrix
        project-container-determinism
        PROPERTIES LABELS project-io-scenario
    )
    set_tests_properties(
        texture-export-formats texture-export-plan texture-export-execution
        texture-export-report-generate texture-export-report-json
        texture-export-determinism
        io-scenario-matrix
        PROPERTIES LABELS "project-io-scenario;texture-export-scenario"
    )
    set_tests_properties(
        preview-emission
        PROPERTIES LABELS "shader-emission-scenario;mesh-maps-scenario"
    )
    set_property(
        TEST version-consistency mesh-maps
        APPEND PROPERTY LABELS project-io-scenario
    )
    set_property(
        TEST
            smart-material-serialization
            smart-mask-instances
            smart-material-resources
            preset-shelf-library
            texture-document
            mesh-map-generators
            smart-material-scenario-matrix
        APPEND PROPERTY LABELS smart-materials-scenario
    )
endif()
