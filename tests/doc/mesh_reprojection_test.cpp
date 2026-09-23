#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/mesh_reprojection.hpp>
#include <ctex/mesh/mesh.hpp>
#include <exception>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;

struct MeshBuffers {
    std::array<mesh::Vec3f, 3> positions{
        mesh::Vec3f{0.0F, 0.0F, 0.0F},
        mesh::Vec3f{1.0F, 0.0F, 0.0F},
        mesh::Vec3f{0.0F, 1.0F, 0.0F},
    };
    std::array<mesh::Vec3f, 3> normals{
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    std::array<mesh::Vec2f, 3> uv{mesh::Vec2f{0.0F, 0.0F}, mesh::Vec2f{1.0F, 0.0F},
                                  mesh::Vec2f{0.0F, 1.0F}};
    std::array<mesh::UvSetView, 1> uv_sets{mesh::UvSetView{"paint", uv}};
    std::array<mesh::MeshPartition, 1> partitions{
        mesh::MeshPartition{mesh::PartitionKind::material, "body", "Body"}};
    std::array<std::uint32_t, 1> face_partitions{0};
    std::array<std::uint32_t, 1> face_materials{0};

    [[nodiscard]] mesh::MeshDescriptor descriptor() const {
        return {.positions = positions,
                .normals = normals,
                .vertex_colors = {},
                .triangle_indices = indices,
                .uv_sets = uv_sets,
                .default_uv_set = "paint",
                .partitions = partitions,
                .face_partition_indices = face_partitions,
                .face_material_ids = face_materials};
    }
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

doc::TextureSet& create_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Body",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "body",
                                        .uv_set = "paint",
                                        .width = 4,
                                        .height = 4,
                                        .default_bit_depth = 8});
}

doc::MeshReprojectionLimits limits() {
    return {.maximum_distance = 0.25,
            .maximum_normal_angle_radians = 0.5,
            .require_visibility = true,
            .visibility_epsilon = 1.0e-5,
            .ambiguity_distance_epsilon = 1.0e-6,
            .maximum_work_items = 10'000,
            .progress_interval = 2};
}

doc::EditableAuthoringEntry surface_path(mesh::MeshRevision revision) {
    doc::EditableAuthoringEntry entry{.identifier = "path",
                                      .kind = doc::EditableEntryKind::surface_path,
                                      .material_identity = "paint-material",
                                      .mesh_revision = revision};
    entry.surface_points = {
        {.position = {0.2, 0.2, 0.0},
         .normal = {0.0, 0.0, 1.0},
         .triangle = 0,
         .barycentric = {0.6, 0.2, 0.2},
         .width = 0.1},
        {.position = {0.4, 0.2, 0.0},
         .normal = {0.0, 0.0, 1.0},
         .triangle = 0,
         .barycentric = {0.4, 0.4, 0.2},
         .width = 0.1},
    };
    entry.dependent_tiles = {{.semantic_id = "pbr.base_color", .tile_x = 0, .tile_y = 0}};
    return entry;
}

bool maps_commits_and_updates_editable_attachments() {
    MeshBuffers source_buffers;
    mesh::MeshBinding source(source_buffers.descriptor());
    MeshBuffers replacement_buffers;
    for (mesh::Vec3f& position : replacement_buffers.positions) {
        position.z = 0.1F;
    }
    replacement_buffers.uv = {mesh::Vec2f{1.0F, 0.0F}, mesh::Vec2f{0.0F, 0.0F},
                              mesh::Vec2f{1.0F, 1.0F}};
    mesh::MeshBinding replacement(replacement_buffers.descriptor());

    doc::TextureDocument document;
    doc::TextureSet& texture_set = create_set(document);
    texture_set.channels().enable("pbr.base_color");
    texture_set.channels().enable("pbr.normal");
    const std::array base_pixel{std::byte{12}, std::byte{34}, std::byte{56}};
    const std::array normal_pixel{std::byte{255}, std::byte{128}, std::byte{128}};
    texture_set.channels().pixels("pbr.base_color").write_pixel(0, 3, base_pixel);
    for (std::uint32_t y = 0; y < 4; ++y) {
        for (std::uint32_t x = 0; x < 4; ++x) {
            texture_set.channels().pixels("pbr.normal").write_pixel(x, y, normal_pixel);
        }
    }
    static_cast<void>(texture_set.editable_authoring().add(surface_path(source.revision())));

    std::vector<std::size_t> progress;
    const doc::MeshReprojectionPreflight preflight = doc::preflight_mesh_reprojection(
        document, source, replacement, limits(),
        {.report_progress = [&](std::size_t completed) { progress.push_back(completed); }});
    const doc::MeshReprojectionCommitReport report = doc::commit_mesh_reprojection(
        document, source, replacement, preflight, doc::ReprojectionHolePolicy::retain_target,
        doc::ReprojectionAmbiguityPolicy::nearest_then_lowest_triangle);
    const doc::EditableAuthoringEntry& path = texture_set.editable_authoring().entry("path");
    const auto& first_mapping = preflight.texels.front();
    const auto converted_normal =
        texture_set.channels().pixels("pbr.normal").read_pixel(first_mapping.x, first_mapping.y);

    return expect(preflight.mapped_texel_count != 0 && preflight.unmapped_texel_count == 0 &&
                      preflight.ambiguous_texel_count == 0,
                  "preflight did not map the destination UV coverage") &&
           expect(preflight.affected_entries.size() == 1 &&
                      preflight.affected_entries.front().unmapped_point_count == 0,
                  "preflight did not report the affected editable path") &&
           expect(!progress.empty() && progress.back() == preflight.tested_candidate_count,
                  "preflight did not publish bounded progress") &&
           expect(report.reprojected_texel_count == preflight.mapped_texel_count * 2 &&
                      report.transformed_tangent_normal_count == preflight.mapped_texel_count,
                  "commit did not stage every mapped channel or transform tangent normals") &&
           expect(std::to_integer<unsigned>(converted_normal[0]) < 8,
                  "tangent normal was not converted into the mirrored destination basis") &&
           expect(path.mesh_revision == preflight.published_revision &&
                      path.surface_points.front().position[2] > 0.09 &&
                      texture_set.editable_authoring().undo_step_count() == 2,
                  "commit did not update editable attachments as one revision-checked edit") &&
           expect(report.reprojected_entries == std::vector<std::string>{"path"},
                  "commit did not report the reprojected editable entry");
}

bool cancellation_and_budget_leave_the_document_unchanged() {
    MeshBuffers source_buffers;
    mesh::MeshBinding source(source_buffers.descriptor());
    MeshBuffers replacement_buffers;
    replacement_buffers.positions[0].z = 0.1F;
    replacement_buffers.positions[1].z = 0.1F;
    replacement_buffers.positions[2].z = 0.1F;
    mesh::MeshBinding replacement(replacement_buffers.descriptor());
    doc::TextureDocument document;
    doc::TextureSet& texture_set = create_set(document);
    texture_set.channels().enable("pbr.base_color");
    const std::array pixel{std::byte{9}, std::byte{8}, std::byte{7}};
    texture_set.channels().pixels("pbr.base_color").write_pixel(0, 3, pixel);

    bool cancelled = false;
    try {
        static_cast<void>(doc::preflight_mesh_reprojection(document, source, replacement, limits(),
                                                           {.is_cancelled = [] { return true; }}));
    } catch (const doc::MeshReprojectionError& error) {
        cancelled = error.code() == doc::MeshReprojectionErrorCode::cancelled;
    }
    doc::MeshReprojectionLimits bounded = limits();
    bounded.maximum_work_items = 1;
    bool over_budget = false;
    try {
        static_cast<void>(doc::preflight_mesh_reprojection(document, source, replacement, bounded));
    } catch (const doc::MeshReprojectionError& error) {
        over_budget = error.code() == doc::MeshReprojectionErrorCode::over_budget;
    }
    const auto actual = texture_set.channels().pixels("pbr.base_color").read_pixel(0, 3);
    return expect(cancelled, "preflight ignored cancellation") &&
           expect(over_budget, "preflight ignored the work-item budget") &&
           expect(std::ranges::equal(actual, pixel),
                  "cancelled or over-budget preflight mutated the document");
}

bool holes_and_ambiguities_require_explicit_policy() {
    MeshBuffers source_buffers;
    mesh::MeshBinding source(source_buffers.descriptor());
    MeshBuffers replacement_buffers;
    for (mesh::Vec3f& position : replacement_buffers.positions) {
        position.z = 0.2F;
    }
    mesh::MeshBinding replacement(replacement_buffers.descriptor());
    doc::TextureDocument document;
    doc::TextureSet& texture_set = create_set(document);
    texture_set.channels().enable("pbr.base_color");

    doc::MeshReprojectionLimits too_short = limits();
    too_short.maximum_distance = 0.05;
    const auto holes = doc::preflight_mesh_reprojection(document, source, replacement, too_short);
    const auto report = doc::commit_mesh_reprojection(document, source, replacement, holes,
                                                      doc::ReprojectionHolePolicy::channel_default,
                                                      doc::ReprojectionAmbiguityPolicy::refuse);
    return expect(holes.unmapped_texel_count != 0 && holes.mapped_texel_count == 0,
                  "distance limit did not report destination holes") &&
           expect(report.defaulted_hole_count == holes.unmapped_texel_count,
                  "explicit channel-default hole policy was not applied");
}

bool stale_source_pixels_are_refused() {
    MeshBuffers source_buffers;
    mesh::MeshBinding source(source_buffers.descriptor());
    MeshBuffers replacement_buffers;
    for (mesh::Vec3f& position : replacement_buffers.positions) {
        position.z = 0.1F;
    }
    mesh::MeshBinding replacement(replacement_buffers.descriptor());
    doc::TextureDocument document;
    doc::TextureSet& texture_set = create_set(document);
    texture_set.channels().enable("pbr.base_color");
    const auto preflight =
        doc::preflight_mesh_reprojection(document, source, replacement, limits());
    const std::array changed{std::byte{4}, std::byte{5}, std::byte{6}};
    texture_set.channels().pixels("pbr.base_color").write_pixel(0, 3, changed);
    bool stale = false;
    try {
        static_cast<void>(doc::commit_mesh_reprojection(
            document, source, replacement, preflight, doc::ReprojectionHolePolicy::retain_target,
            doc::ReprojectionAmbiguityPolicy::nearest_then_lowest_triangle));
    } catch (const doc::MeshReprojectionError& error) {
        stale = error.code() == doc::MeshReprojectionErrorCode::stale_preflight;
    }
    return expect(stale, "commit accepted source pixels changed after preflight") &&
           expect(std::ranges::equal(
                      texture_set.channels().pixels("pbr.base_color").read_pixel(0, 3), changed),
                  "stale commit changed the document");
}

bool ambiguous_surfaces_require_resolution_policy() {
    std::array<mesh::Vec3f, 6> positions{
        mesh::Vec3f{0.0F, 0.0F, 0.0F}, mesh::Vec3f{1.0F, 0.0F, 0.0F},
        mesh::Vec3f{0.0F, 1.0F, 0.0F}, mesh::Vec3f{0.0F, 0.0F, 0.0F},
        mesh::Vec3f{1.0F, 0.0F, 0.0F}, mesh::Vec3f{0.0F, 1.0F, 0.0F}};
    std::array<mesh::Vec3f, 6> normals{
        mesh::Vec3f{0.0F, 0.0F, 1.0F}, mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F}, mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F}, mesh::Vec3f{0.0F, 0.0F, 1.0F}};
    std::array<mesh::Vec2f, 6> uv{mesh::Vec2f{0.0F, 0.0F}, mesh::Vec2f{1.0F, 0.0F},
                                  mesh::Vec2f{0.0F, 1.0F}, mesh::Vec2f{0.0F, 0.0F},
                                  mesh::Vec2f{1.0F, 0.0F}, mesh::Vec2f{0.0F, 1.0F}};
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<mesh::UvSetView, 1> uv_sets{mesh::UvSetView{"paint", uv}};
    std::array<mesh::MeshPartition, 1> partitions{
        mesh::MeshPartition{mesh::PartitionKind::material, "body", "Body"}};
    std::array<std::uint32_t, 2> face_partitions{0, 0};
    std::array<std::uint32_t, 2> face_materials{0, 0};
    mesh::MeshBinding source({.positions = positions,
                              .normals = normals,
                              .vertex_colors = {},
                              .triangle_indices = indices,
                              .uv_sets = uv_sets,
                              .default_uv_set = "paint",
                              .partitions = partitions,
                              .face_partition_indices = face_partitions,
                              .face_material_ids = face_materials});
    MeshBuffers replacement_buffers;
    mesh::MeshBinding replacement(replacement_buffers.descriptor());
    doc::TextureDocument document;
    doc::TextureSet& texture_set = create_set(document);
    texture_set.channels().enable("pbr.base_color");
    const auto preflight =
        doc::preflight_mesh_reprojection(document, source, replacement, limits());
    bool refused = false;
    try {
        static_cast<void>(doc::commit_mesh_reprojection(document, source, replacement, preflight,
                                                        doc::ReprojectionHolePolicy::retain_target,
                                                        doc::ReprojectionAmbiguityPolicy::refuse));
    } catch (const doc::MeshReprojectionError& error) {
        refused = error.code() == doc::MeshReprojectionErrorCode::unresolved_ambiguity;
    }
    const auto resolved = doc::commit_mesh_reprojection(
        document, source, replacement, preflight, doc::ReprojectionHolePolicy::retain_target,
        doc::ReprojectionAmbiguityPolicy::nearest_then_lowest_triangle);
    return expect(preflight.ambiguous_texel_count != 0,
                  "thin duplicate surfaces were not reported as ambiguous") &&
           expect(refused, "commit accepted ambiguity without a resolution policy") &&
           expect(resolved.resolved_ambiguity_count == preflight.ambiguous_texel_count,
                  "explicit ambiguity policy did not resolve every mapped channel sample");
}

}  // namespace

int main() {
    try {
        return maps_commits_and_updates_editable_attachments() &&
                       cancellation_and_budget_leave_the_document_unchanged() &&
                       holes_and_ambiguities_require_explicit_policy() &&
                       stale_source_pixels_are_refused() &&
                       ambiguous_surfaces_require_resolution_policy()
                   ? 0
                   : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
