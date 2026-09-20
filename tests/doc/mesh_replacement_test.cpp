#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/doc/mesh_replacement.hpp>
#include <ctex/mesh/mesh.hpp>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::doc;
using namespace ctex::mesh;

struct MeshBuffers {
    std::array<Vec3f, 6> positions{
        Vec3f{0.0F, 0.0F, 0.0F}, Vec3f{1.0F, 0.0F, 0.0F}, Vec3f{0.0F, 1.0F, 0.0F},
        Vec3f{2.0F, 0.0F, 0.0F}, Vec3f{3.0F, 0.0F, 0.0F}, Vec3f{2.0F, 1.0F, 0.0F},
    };
    std::array<Vec3f, 6> normals{
        Vec3f{0.0F, 0.0F, 1.0F}, Vec3f{0.0F, 0.0F, 1.0F}, Vec3f{0.0F, 0.0F, 1.0F},
        Vec3f{0.0F, 0.0F, 1.0F}, Vec3f{0.0F, 0.0F, 1.0F}, Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<Vec2f, 6> uv{
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
    };
    std::array<UvSetView, 1> uv_sets{UvSetView{"paint", uv}};
    std::array<MeshPartition, 2> partitions{
        MeshPartition{PartitionKind::material, "body", "Body"},
        MeshPartition{PartitionKind::material, "trim", "Trim"},
    };
    std::array<std::uint32_t, 2> face_partitions{0, 1};
    std::array<std::uint32_t, 2> face_materials{0, 1};

    [[nodiscard]] MeshDescriptor descriptor() const {
        return {
            .positions = positions,
            .normals = normals,
            .vertex_colors = {},
            .triangle_indices = indices,
            .uv_sets = uv_sets,
            .default_uv_set = "paint",
            .partitions = partitions,
            .face_partition_indices = face_partitions,
            .face_material_ids = face_materials,
        };
    }
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Operation>
bool expect_invalid_argument(Operation&& operation, std::string_view message) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (const std::exception& error) {
        std::cerr << message << ": wrong exception: " << error.what() << '\n';
        return false;
    }
    std::cerr << message << ": no exception\n";
    return false;
}

TextureSetDescriptor texture_set_descriptor(std::string partition) {
    return {
        .display_name = partition,
        .partition_kind = PartitionSourceKind::material,
        .partition_key = std::move(partition),
        .uv_set = "paint",
        .width = 8,
        .height = 8,
        .default_bit_depth = 8,
    };
}

std::string create_painted_set(TextureDocument& document, std::string partition,
                               std::array<std::byte, 3> pixel) {
    const std::string identifier =
        document.create_texture_set(texture_set_descriptor(std::move(partition))).id();
    TextureChannels& channels = document.texture_set(identifier).channels();
    channels.enable("pbr.base_color");
    channels.pixels("pbr.base_color").write_pixel(2, 3, pixel);
    return identifier;
}

bool pixel_equals(const TextureDocument& document, std::string_view identifier,
                  std::span<const std::byte> expected) {
    const auto actual =
        document.texture_set(identifier).channels().pixels("pbr.base_color").read_pixel(2, 3);
    return std::ranges::equal(actual, expected);
}

const TextureSetMeshReplacement& result_for(const MeshReplacementPlan& plan,
                                            std::string_view identifier) {
    const auto found = std::ranges::find(plan.texture_sets(), identifier,
                                         &TextureSetMeshReplacement::texture_set_id);
    if (found == plan.texture_sets().end()) {
        throw std::runtime_error("mesh replacement report omitted a texture set");
    }
    return *found;
}

bool stable_identity_ignores_mesh_and_face_order() {
    MeshBuffers source_buffers;
    MeshBinding source(source_buffers.descriptor());
    TextureDocument document;
    const std::string body =
        create_painted_set(document, "body", {std::byte{10}, std::byte{20}, std::byte{30}});
    const std::string trim =
        create_painted_set(document, "trim", {std::byte{40}, std::byte{50}, std::byte{60}});

    MeshBuffers replacement_buffers;
    replacement_buffers.positions[0].z = 4.0F;
    replacement_buffers.indices = {5, 4, 3, 2, 1, 0};
    replacement_buffers.partitions = {
        MeshPartition{PartitionKind::material, "trim", "Renamed trim"},
        MeshPartition{PartitionKind::material, "body", "Renamed body"},
    };
    replacement_buffers.face_partitions = {0, 1};
    const MeshView replacement(replacement_buffers.descriptor());
    const MeshReplacementPlan plan = analyze_mesh_replacement(document, source, replacement);
    const MeshReplacementApplyReport applied =
        apply_mesh_replacement_policies(document, source, plan, {});

    return expect(plan.source_revision() == source.revision(),
                  "plan omitted the source revision") &&
           expect(result_for(plan, body).change == MeshUvChange::unchanged &&
                      result_for(plan, trim).change == MeshUvChange::unchanged,
                  "stable partition matching depended on mesh, partition or face order") &&
           expect(applied.replacement_ready && applied.kept_texture_sets.size() == 2 &&
                      applied.cleared_texture_sets.empty() &&
                      applied.reprojection_pending_texture_sets.empty(),
                  "unchanged UV layouts did not keep both texture sets");
}

bool policies_preserve_or_clear_pixels() {
    MeshBuffers source_buffers;
    MeshBinding source(source_buffers.descriptor());
    TextureDocument document;
    const std::array painted{std::byte{10}, std::byte{20}, std::byte{30}};
    const std::string body = create_painted_set(document, "body", painted);

    MeshBuffers replacement_buffers;
    replacement_buffers.uv[0] = {0.25F, 0.25F};
    const MeshView replacement(replacement_buffers.descriptor());
    const MeshReplacementPlan plan = analyze_mesh_replacement(document, source, replacement);
    const TextureSetMeshReplacement& change = result_for(plan, body);
    const std::array keep{MeshReplacementDecision{body, MeshReplacementPolicy::keep_texels}};
    const MeshReplacementApplyReport kept =
        apply_mesh_replacement_policies(document, source, plan, keep);
    const std::array reprojection{
        MeshReplacementDecision{body, MeshReplacementPolicy::request_reprojection}};
    const MeshReplacementApplyReport pending =
        apply_mesh_replacement_policies(document, source, plan, reprojection);
    const bool reprojection_preserved =
        expect(change.change == MeshUvChange::changed && change.source_face_count == 1 &&
                   change.replacement_face_count == 1,
               "changed UV layout was not reported") &&
        expect(kept.replacement_ready && kept.kept_texture_sets == std::vector<std::string>{body} &&
                   pixel_equals(document, body, painted),
               "keep policy did not preserve changed-layout pixels") &&
        expect(!pending.replacement_ready &&
                   pending.reprojection_pending_texture_sets == std::vector<std::string>{body},
               "reprojection policy was not returned as pending") &&
        expect(pixel_equals(document, body, painted), "reprojection request changed source pixels");

    const auto revision_before =
        document.texture_set(body).channels().pixels("pbr.base_color").revision_cursor();
    const std::array clear{MeshReplacementDecision{body, MeshReplacementPolicy::clear}};
    const MeshReplacementApplyReport cleared =
        apply_mesh_replacement_policies(document, source, plan, clear);
    const auto& channels = document.texture_set(body).channels();
    const std::array default_pixel{std::byte{128}, std::byte{128}, std::byte{128}};
    return reprojection_preserved &&
           expect(cleared.cleared_texture_sets == std::vector<std::string>{body},
                  "clear policy did not identify the cleared texture set") &&
           expect(channels.is_enabled("pbr.base_color") && channels.resident_pixel_bytes() == 0,
                  "clear policy disabled the channel or retained its tiles") &&
           expect(pixel_equals(document, body, default_pixel),
                  "clear policy did not restore the channel default") &&
           expect(channels.pixels("pbr.base_color").revision_cursor().epoch > revision_before.epoch,
                  "clear policy did not invalidate prior revision cursors");
}

bool invalid_decisions_are_transactional() {
    MeshBuffers source_buffers;
    MeshBinding source(source_buffers.descriptor());
    TextureDocument document;
    const std::array body_pixel{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array trim_pixel{std::byte{4}, std::byte{5}, std::byte{6}};
    const std::string body = create_painted_set(document, "body", body_pixel);
    const std::string trim = create_painted_set(document, "trim", trim_pixel);

    MeshBuffers replacement_buffers;
    replacement_buffers.uv[0] = {0.2F, 0.2F};
    replacement_buffers.uv[3] = {0.3F, 0.3F};
    const MeshView replacement(replacement_buffers.descriptor());
    const MeshReplacementPlan plan = analyze_mesh_replacement(document, source, replacement);
    const std::array incomplete{MeshReplacementDecision{body, MeshReplacementPolicy::clear}};
    const bool missing_refused = expect_invalid_argument(
        [&] {
            static_cast<void>(apply_mesh_replacement_policies(document, source, plan, incomplete));
        },
        "missing decision was accepted");
    const std::array invalid{
        MeshReplacementDecision{body, static_cast<MeshReplacementPolicy>(255)},
        MeshReplacementDecision{trim, MeshReplacementPolicy::keep_texels},
    };
    const bool invalid_refused = expect_invalid_argument(
        [&] {
            static_cast<void>(apply_mesh_replacement_policies(document, source, plan, invalid));
        },
        "invalid policy was accepted");
    const std::array pending{
        MeshReplacementDecision{body, MeshReplacementPolicy::clear},
        MeshReplacementDecision{trim, MeshReplacementPolicy::request_reprojection},
    };
    const MeshReplacementApplyReport pending_report =
        apply_mesh_replacement_policies(document, source, plan, pending);

    return missing_refused && invalid_refused &&
           expect(!pending_report.replacement_ready &&
                      pending_report.cleared_texture_sets.empty() &&
                      pending_report.reprojection_pending_texture_sets ==
                          std::vector<std::string>{trim},
                  "mixed policy plan did not remain pending") &&
           expect(
               pixel_equals(document, body, body_pixel) && pixel_equals(document, trim, trim_pixel),
               "rejected decision set partially mutated channel pixels");
}

bool missing_identity_and_uv_are_reported() {
    MeshBuffers source_buffers;
    MeshBinding source(source_buffers.descriptor());
    TextureDocument document;
    const std::string missing_partition =
        create_painted_set(document, "missing", {std::byte{1}, std::byte{1}, std::byte{1}});

    MeshBuffers replacement_buffers;
    const MeshView replacement(replacement_buffers.descriptor());
    const MeshReplacementPlan source_missing =
        analyze_mesh_replacement(document, source, replacement);

    TextureDocument replacement_document;
    const std::string replacement_missing = create_painted_set(
        replacement_document, "body", {std::byte{2}, std::byte{2}, std::byte{2}});
    MeshBuffers no_body_buffers;
    no_body_buffers.partitions[0] = MeshPartition{PartitionKind::material, "other", "Other"};
    const MeshView no_body(no_body_buffers.descriptor());
    const MeshReplacementPlan no_body_plan =
        analyze_mesh_replacement(replacement_document, source, no_body);

    TextureDocument uv_document;
    TextureSetDescriptor descriptor = texture_set_descriptor("body");
    descriptor.uv_set = "detail";
    const std::string missing_uv = uv_document.create_texture_set(std::move(descriptor)).id();
    const MeshReplacementPlan uv_missing =
        analyze_mesh_replacement(uv_document, source, replacement);

    return expect(result_for(source_missing, missing_partition).change ==
                      MeshUvChange::source_partition_missing,
                  "missing source partition was not reported") &&
           expect(result_for(no_body_plan, replacement_missing).change ==
                      MeshUvChange::replacement_partition_missing,
                  "missing replacement partition was not reported") &&
           expect(result_for(uv_missing, missing_uv).change == MeshUvChange::uv_set_missing,
                  "missing bound UV set was not reported");
}

}  // namespace

int main() {
    return stable_identity_ignores_mesh_and_face_order() && policies_preserve_or_clear_pixels() &&
                   invalid_decisions_are_transactional() && missing_identity_and_uv_are_reported()
               ? 0
               : 1;
}
