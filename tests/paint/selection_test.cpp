#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/paint/selection.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::paint;

constexpr pick::Mat4f identity{
    {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
     1.0F},
};

class MeshBuffers {
public:
    MeshBuffers()
        : positions_{{-0.8F, -0.2F, 0.0F}, {-0.4F, -0.2F, 0.0F}, {-0.6F, 0.2F, 0.0F},
                     {-0.2F, 0.0F, 0.0F},  {0.2F, 0.0F, 0.0F},   {0.0F, 0.4F, 0.0F},
                     {0.4F, -0.2F, 0.0F},  {0.8F, -0.2F, 0.0F},  {0.6F, 0.2F, 0.0F}},
          normals_(positions_.size(), {0.0F, 0.0F, 1.0F}),
          uv_(positions_.size(), {0.0F, 0.0F}),
          indices_{0, 1, 2, 3, 4, 5, 6, 7, 8},
          face_partitions_{0, 0, 0},
          face_materials_{1, 1, 1} {
        uv_sets_[0] = {"paint", uv_};
    }

    [[nodiscard]] mesh::MeshDescriptor descriptor() const {
        return {.positions = positions_,
                .normals = normals_,
                .vertex_colors = {},
                .triangle_indices = indices_,
                .uv_sets = uv_sets_,
                .default_uv_set = "paint",
                .partitions = partitions_,
                .face_partition_indices = face_partitions_,
                .face_material_ids = face_materials_};
    }

private:
    std::vector<mesh::Vec3f> positions_;
    std::vector<mesh::Vec3f> normals_;
    std::vector<mesh::Vec2f> uv_;
    std::vector<std::uint32_t> indices_;
    std::vector<std::uint32_t> face_partitions_;
    std::vector<std::uint32_t> face_materials_;
    std::array<mesh::UvSetView, 1> uv_sets_{};
    std::array<mesh::MeshPartition, 1> partitions_{
        mesh::MeshPartition{mesh::PartitionKind::object, "selection", "Selection"}};
};

SurfaceTexel texel(std::uint32_t triangle, Vec3d position = {}) {
    return {.position = position,
            .normal = {0.0, 0.0, 1.0},
            .geometric_normal = {0.0, 0.0, 1.0},
            .uv = {},
            .triangle = triangle};
}

CachedSurfaceMaps screen_surface(mesh::MeshRevision revision) {
    return {.texture_set_id = "object:selection:paint",
            .uv_set = "paint",
            .mesh_revision = revision,
            .surface = {.width = 4,
                        .height = 1,
                        .tile_origin = {},
                        .texels = {texel(0, {-0.6, 0.0, 0.0}), texel(1, {0.0, 0.0, 0.0}),
                                   texel(1, {-0.2, 0.0, 0.0}), texel(2, {0.6, 0.0, 0.0})}},
            .coverage = {1, 1, 1, 1},
            .triangle_identity = {0, 1, 1, 2},
            .uv_island_identity = {10, 10, 10, 20}};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

RejectedCoverageRaster rejected() {
    return {.coverage = {.width = 4, .height = 1, .values = {1.0, 1.0, 1.0, 1.0}},
            .stamp_events = {{.stamp_ordinal = 0, .values = {1.0, 1.0, 1.0, 1.0}}},
            .report = {}};
}

bool rectangle_and_lasso_select_screen_regions() {
    MeshBuffers buffers;
    const mesh::MeshBinding mesh(buffers.descriptor());
    pick::SpatialIndex index(mesh);
    const CachedSurfaceMaps surface = screen_surface(mesh.revision());
    const pick::ScreenRegionView view{{100, 100}, identity, identity};
    const SelectionResult rectangle =
        select_screen_rectangle(index, mesh, surface, {{48.0F, 48.0F}, {52.0F, 52.0F}}, view);
    constexpr std::array lasso{
        pick::ScreenPosition{5.0F, 35.0F}, pick::ScreenPosition{35.0F, 35.0F},
        pick::ScreenPosition{25.0F, 50.0F}, pick::ScreenPosition{35.0F, 65.0F},
        pick::ScreenPosition{5.0F, 65.0F}};
    const SelectionResult selected = select_screen_lasso(index, mesh, surface, lasso, view);
    const RejectedCoverageRaster restricted =
        apply_paint_masks(rejected(), {.active_layer_masks = {},
                                       .colour_id_selection = std::nullopt,
                                       .geometry_selection = std::nullopt,
                                       .screen_selection = selected.as_paint_restriction(),
                                       .uv_island_selection = std::nullopt});
    return expect(rectangle.values == std::vector<double>({0.0, 1.0, 0.0, 0.0}) &&
                      rectangle.selected_triangle_ids == std::vector<std::uint32_t>({1}),
                  "rectangle selection did not map its screen region to texture texels") &&
           expect(selected.values == std::vector<double>({1.0, 0.0, 0.0, 0.0}) &&
                      selected.kind == SelectionKind::screen_lasso,
                  "lasso selection did not map its screen region to texture texels") &&
           expect(restricted.coverage.values == std::vector<double>({1.0, 0.0, 0.0, 0.0}),
                  "active lasso did not reject paint outside its region");
}

CachedSurfaceMaps polygon_surface() {
    return {.texture_set_id = "object:selection:paint",
            .uv_set = "paint",
            .mesh_revision = 1,
            .surface = {.width = 6,
                        .height = 1,
                        .tile_origin = {},
                        .texels = {texel(0), texel(0), texel(1), texel(1), texel(2), texel(2)}},
            .coverage = {1, 1, 1, 1, 1, 1},
            .triangle_identity = {0, 0, 1, 1, 2, 2},
            .uv_island_identity = {10, 10, 10, 10, 20, 20}};
}

std::vector<FillTriangleTopology> topology() {
    return {
        {.triangle_identity = 0, .geometric_normal = {0.0, 0.0, 1.0}, .adjacent_triangles = {1}},
        {.triangle_identity = 1,
         .geometric_normal = {0.0, 0.5, 0.8660254037844386},
         .adjacent_triangles = {0, 2}},
        {.triangle_identity = 2, .geometric_normal = {0.0, 1.0, 0.0}, .adjacent_triangles = {1}}};
}

bool polygon_modes_and_stored_mask_preserve_regions() {
    const CachedSurfaceMaps surface = polygon_surface();
    const std::vector graph = topology();
    const SelectionResult triangle =
        select_polygon(surface, {.mode = PolygonSelectionMode::triangle,
                                 .picked_texel = 0,
                                 .maximum_angle_degrees = default_fill_angle_degrees,
                                 .triangle_topology = {}});
    const SelectionResult island =
        select_polygon(surface, {.mode = PolygonSelectionMode::uv_island,
                                 .picked_texel = 0,
                                 .maximum_angle_degrees = default_fill_angle_degrees,
                                 .triangle_topology = {}});
    SelectionResult connected =
        select_polygon(surface, {.mode = PolygonSelectionMode::connected_by_angle,
                                 .picked_texel = 0,
                                 .maximum_angle_degrees = 45.0,
                                 .triangle_topology = graph});
    const SelectionResult clamped =
        select_polygon(surface, {.mode = PolygonSelectionMode::connected_by_angle,
                                 .picked_texel = 0,
                                 .maximum_angle_degrees = -1.0,
                                 .triangle_topology = graph});
    const StoredSelectionMask stored = store_selection_mask(connected);
    connected.values[0] = 0.0;
    return expect(triangle.values == std::vector<double>({1, 1, 0, 0, 0, 0}) &&
                      triangle.selected_triangle_ids == std::vector<std::uint32_t>({0}),
                  "triangle polygon selection included another triangle") &&
           expect(island.values == std::vector<double>({1, 1, 1, 1, 0, 0}) &&
                      island.selected_triangle_ids == std::vector<std::uint32_t>({0, 1}),
                  "UV-island polygon selection crossed an island boundary") &&
           expect(stored.values == std::vector<double>({1, 1, 1, 1, 0, 0}) &&
                      stored.view().values.data() == stored.values.data(),
                  "stored selection mask did not own an independent region copy") &&
           expect(clamped.selected_triangle_ids == std::vector<std::uint32_t>({0}) &&
                      clamped.parameter_report.clamp_for("paint.connected.maximum_angle_degrees") ==
                          ToolParameterClamp{.name = "paint.connected.maximum_angle_degrees",
                                             .supplied = -1.0,
                                             .resolved = 0.0},
                  "polygon selection did not propagate its resolved angle and clamp report");
}

bool stale_and_inconsistent_inputs_are_refused() {
    MeshBuffers buffers;
    const mesh::MeshBinding mesh(buffers.descriptor());
    pick::SpatialIndex index(mesh);
    const CachedSurfaceMaps stale = screen_surface(mesh.revision() + 1);
    const pick::ScreenRegionView view{{100, 100}, identity, identity};
    bool stale_refused = false;
    try {
        static_cast<void>(
            select_screen_rectangle(index, mesh, stale, {{48.0F, 48.0F}, {52.0F, 52.0F}}, view));
    } catch (const std::invalid_argument&) {
        stale_refused = true;
    }
    const SelectionResult inconsistent{.width = 2,
                                       .height = 1,
                                       .kind = SelectionKind::screen_lasso,
                                       .parameter_report = {},
                                       .values = {1.0, 0.5},
                                       .selected_triangle_ids = {0},
                                       .selected_texel_count = 1};
    bool storage_refused = false;
    try {
        static_cast<void>(store_selection_mask(inconsistent));
    } catch (const std::invalid_argument&) {
        storage_refused = true;
    }
    return expect(stale_refused && storage_refused,
                  "stale screen selection or inconsistent stored mask was not refused");
}

}  // namespace

int main() {
    return rectangle_and_lasso_select_screen_regions() &&
                   polygon_modes_and_stored_mask_preserve_regions() &&
                   stale_and_inconsistent_inputs_are_refused()
               ? 0
               : 1;
}
