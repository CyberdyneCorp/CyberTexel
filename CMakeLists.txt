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
)
ctex_add_module(emit DEPENDS graph image)
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
ctex_add_module(exec DEPENDS emit image)
ctex_add_module(capi DEPENDS image mesh pick graph emit doc paint maps xport io exec)

ctex_add_library()
target_link_libraries(cybertexel PUBLIC ctex_lodepng)

include(CTest)
if(BUILD_TESTING)
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

    add_executable(ctex_material_graph_output_test tests/doc/material_graph_test.cpp)
    ctex_configure_cpp_target(ctex_material_graph_output_test)
    target_link_libraries(
        ctex_material_graph_output_test
        PRIVATE ctex_doc ctex_mesh ctex_graph ctex_image
    )
    add_test(NAME material-graph-output COMMAND ctex_material_graph_output_test)
endif()
