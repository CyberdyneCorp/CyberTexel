cmake_minimum_required(VERSION 3.24)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CTEX_VERSION)
string(STRIP "${CTEX_VERSION}" CTEX_VERSION)
if(NOT CTEX_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a three-part semantic version")
endif()

project(CyberTexel VERSION "${CTEX_VERSION}" LANGUAGES C CXX)

option(CTEX_WARNINGS_AS_ERRORS "Treat compiler warnings in CyberTexel sources as errors" ON)
option(CTEX_ENABLE_SANITIZERS "Enable address and undefined-behaviour sanitizers" OFF)

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
    SOURCES src/doc/channels.cpp src/doc/document.cpp src/doc/material_graph.cpp
)
ctex_add_module(paint DEPENDS doc emit pick)
ctex_add_module(maps DEPENDS image doc)
ctex_add_module(xport DEPENDS doc)
ctex_add_module(io DEPENDS doc image)
target_link_libraries(ctex_io PRIVATE ctex_lodepng)
ctex_add_module(
    exec
    DEPENDS emit image
    SOURCES
        src/exec/exec.cpp
        src/exec/cpu_reference.cpp
        src/exec/host_execution.cpp
        src/exec/emission_features.cpp
)
ctex_add_module(capi DEPENDS image mesh pick graph emit doc paint maps xport io exec)

ctex_add_library()
target_link_libraries(cybertexel PUBLIC ctex_lodepng ctex_kong)

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

    add_executable(ctex_channels_test tests/doc/channels_test.cpp)
    ctex_configure_cpp_target(ctex_channels_test)
    target_link_libraries(ctex_channels_test PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image)
    add_test(NAME document-channels COMMAND ctex_channels_test)

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
endif()
