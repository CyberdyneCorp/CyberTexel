#include <algorithm>
#include <array>
#include <cstdint>
#include <ctex/doc/document.hpp>
#include <ctex/mesh/mesh.hpp>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

using ctex::mesh::MeshDescriptor;
using ctex::mesh::MeshPartition;
using ctex::mesh::MeshView;
using ctex::mesh::PartitionKind;
using ctex::mesh::TangentFrameDescriptor;
using ctex::mesh::TangentFrameSource;
using ctex::mesh::UvSetView;
using ctex::mesh::Vec2f;
using ctex::mesh::Vec3f;
using ctex::mesh::Vec4f;

struct MeshBuffers {
    std::array<Vec3f, 4> positions{
        Vec3f{-1.0F, -1.0F, 0.0F},
        Vec3f{1.0F, -1.0F, 0.0F},
        Vec3f{1.0F, 1.0F, 0.0F},
        Vec3f{-1.0F, 1.0F, 0.0F},
    };
    std::array<Vec3f, 4> normals{
        Vec3f{0.0F, 0.0F, 1.0F},
        Vec3f{0.0F, 0.0F, 1.0F},
        Vec3f{0.0F, 0.0F, 1.0F},
        Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<Vec4f, 4> colors{
        Vec4f{1.0F, 0.0F, 0.0F, 1.0F},
        Vec4f{0.0F, 1.0F, 0.0F, 1.0F},
        Vec4f{0.0F, 0.0F, 1.0F, 1.0F},
        Vec4f{1.0F, 1.0F, 1.0F, 1.0F},
    };
    std::array<std::uint32_t, 6> indices{0, 1, 2, 0, 2, 3};
    std::array<Vec2f, 4> uv0{
        Vec2f{0.0F, 0.0F},
        Vec2f{1.0F, 0.0F},
        Vec2f{1.0F, 1.0F},
        Vec2f{0.0F, 1.0F},
    };
    std::array<Vec2f, 4> uv1{
        Vec2f{0.1F, 0.1F},
        Vec2f{0.9F, 0.1F},
        Vec2f{0.9F, 0.9F},
        Vec2f{0.1F, 0.9F},
    };
    std::array<Vec2f, 4> uv2 = uv0;
    std::array<Vec2f, 4> uv3 = uv1;
    std::array<UvSetView, 4> uv_sets{
        UvSetView{"paint", uv0},
        UvSetView{"lightmap", uv1},
        UvSetView{"detail", uv2},
        UvSetView{"effects", uv3},
    };
    std::array<MeshPartition, 2> partitions{
        MeshPartition{PartitionKind::material, "body", "Body"},
        MeshPartition{PartitionKind::material, "trim", "Trim"},
    };
    std::array<std::uint32_t, 2> face_partitions{0, 1};
    std::array<std::uint32_t, 2> face_materials{17, 23};

    [[nodiscard]] MeshDescriptor descriptor() const {
        return {
            .positions = positions,
            .normals = normals,
            .vertex_colors = colors,
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
bool expect_invalid_argument(Operation operation, std::string_view message) {
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

bool accepts_in_memory_attributes_without_modification() {
    MeshBuffers buffers;
    const auto positions_before = buffers.positions;
    const auto normals_before = buffers.normals;
    const auto colors_before = buffers.colors;
    const auto indices_before = buffers.indices;
    const auto uv0_before = buffers.uv0;
    const auto uv1_before = buffers.uv1;

    const MeshView mesh(buffers.descriptor());
    const auto attributes = mesh.attributes();

    return expect(attributes.vertex_count == 4, "mesh reported the wrong vertex count") &&
           expect(attributes.triangle_count == 2, "mesh reported the wrong triangle count") &&
           expect(attributes.uv_set_count == 4, "mesh did not retain four named UV sets") &&
           expect(attributes.has_vertex_colors, "mesh omitted its vertex-colour attribute") &&
           expect(attributes.tangent_source == TangentFrameSource::generated &&
                      attributes.corner_tangent_count == buffers.indices.size(),
                  "mesh omitted its generated per-corner tangent frames") &&
           expect(std::ranges::equal(mesh.uv_set("lightmap").values, buffers.uv1),
                  "named UV lookup returned the wrong buffer") &&
           expect(mesh.partition_for_face(0).stable_key == "body",
                  "first face received the wrong partition") &&
           expect(mesh.partition_for_face(1).stable_key == "trim",
                  "second face received the wrong partition") &&
           expect(buffers.positions == positions_before && buffers.normals == normals_before &&
                      buffers.colors == colors_before && buffers.indices == indices_before &&
                      buffers.uv0 == uv0_before && buffers.uv1 == uv1_before,
                  "mesh ingest modified a caller-owned buffer");
}

bool restores_persisted_revision_and_advances_replacements() {
    MeshBuffers buffers;
    constexpr ctex::mesh::MeshRevision persisted_revision = 10'000;
    ctex::mesh::MeshBinding restored =
        ctex::mesh::MeshBinding::restore(buffers.descriptor(), persisted_revision);
    if (!expect(restored.revision() == persisted_revision,
                "restored mesh binding changed its persisted revision")) {
        return false;
    }
    restored.replace(buffers.descriptor());
    const bool advanced = expect(restored.revision() > persisted_revision,
                                 "mesh replacement did not advance beyond the restored revision");
    return advanced &&
           expect_invalid_argument(
               [&] {
                   static_cast<void>(ctex::mesh::MeshBinding::restore(buffers.descriptor(), 0));
               },
               "mesh binding accepted a zero restored revision");
}

bool accepts_declared_supplied_corner_tangents() {
    MeshBuffers buffers;
    const std::array<Vec4f, 6> tangents{
        Vec4f{1.0F, 0.0F, 0.0F, 1.0F}, Vec4f{1.0F, 0.0F, 0.0F, 1.0F}, Vec4f{1.0F, 0.0F, 0.0F, 1.0F},
        Vec4f{1.0F, 0.0F, 0.0F, 1.0F}, Vec4f{1.0F, 0.0F, 0.0F, 1.0F}, Vec4f{1.0F, 0.0F, 0.0F, 1.0F},
    };
    auto descriptor = buffers.descriptor();
    descriptor.corner_tangents = tangents;
    descriptor.tangent_frame =
        TangentFrameDescriptor{.algorithm = ctex::mesh::TangentBasisAlgorithm::mikktspace,
                               .algorithm_version = 1,
                               .uv_set = "paint"};
    const MeshView mesh(descriptor);
    const ctex::mesh::TangentFrameView frames = mesh.tangent_frames();
    return expect(frames.source == TangentFrameSource::supplied && descriptor.tangent_frame &&
                      frames.descriptor == *descriptor.tangent_frame &&
                      std::ranges::equal(frames.corner_tangents, tangents),
                  "declared supplied per-corner tangents were not retained");
}

bool generated_tangents_preserve_mirrored_uv_handedness() {
    const std::array<Vec3f, 6> positions{
        Vec3f{0.0F, 0.0F, 0.0F}, Vec3f{1.0F, 0.0F, 0.0F}, Vec3f{0.0F, 1.0F, 0.0F},
        Vec3f{2.0F, 0.0F, 0.0F}, Vec3f{3.0F, 0.0F, 0.0F}, Vec3f{2.0F, 1.0F, 0.0F},
    };
    std::array<Vec3f, 6> normals{};
    normals.fill({0.0F, 0.0F, 1.0F});
    const std::array<Vec2f, 6> uv{
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{0.0F, 1.0F}, Vec2f{1.0F, 0.0F},
    };
    const std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    const std::array uv_sets{UvSetView{"paint", uv}};
    const std::array partitions{MeshPartition{PartitionKind::material, "body", "Body"}};
    const std::array<std::uint32_t, 2> face_partitions{0, 0};
    const std::array<std::uint32_t, 2> face_materials{1, 1};
    const MeshView mesh({.positions = positions,
                         .normals = normals,
                         .vertex_colors = {},
                         .triangle_indices = indices,
                         .uv_sets = uv_sets,
                         .default_uv_set = "paint",
                         .partitions = partitions,
                         .face_partition_indices = face_partitions,
                         .face_material_ids = face_materials});
    const ctex::mesh::TangentFrameView frames = mesh.tangent_frames();
    const Vec3f regular = ctex::mesh::tangent_space_to_object({0.0F, 1.0F, 0.0F}, normals[0],
                                                              frames.corner_tangents[0]);
    const Vec3f mirrored = ctex::mesh::tangent_space_to_object({0.0F, 1.0F, 0.0F}, normals[3],
                                                               frames.corner_tangents[3]);
    return expect(
               frames.descriptor.algorithm ==
                       ctex::mesh::TangentBasisAlgorithm::ctex_uv_derivative &&
                   frames.descriptor.algorithm_version == ctex::mesh::ctex_uv_derivative_version &&
                   frames.corner_tangents[0].w == 1.0F && frames.corner_tangents[3].w == -1.0F,
               "generated tangent frames lost the mirrored island sign") &&
           expect(regular == Vec3f{0.0F, 1.0F, 0.0F} && mirrored == Vec3f{1.0F, 0.0F, 0.0F},
                  "normal transformation ignored mirrored tangent handedness");
}

bool refuses_undeclared_or_invalid_supplied_tangents() {
    MeshBuffers buffers;
    std::array<Vec4f, 6> tangents{};
    tangents.fill({1.0F, 0.0F, 0.0F, 1.0F});
    auto undeclared = buffers.descriptor();
    undeclared.corner_tangents = tangents;
    auto wrong_count = buffers.descriptor();
    wrong_count.corner_tangents = std::span(tangents).first(5);
    wrong_count.tangent_frame = TangentFrameDescriptor{.uv_set = "paint"};
    auto non_orthogonal = buffers.descriptor();
    auto non_orthogonal_tangents = tangents;
    non_orthogonal_tangents[0] = {0.0F, 0.0F, 1.0F, 1.0F};
    non_orthogonal.corner_tangents = non_orthogonal_tangents;
    non_orthogonal.tangent_frame = TangentFrameDescriptor{.uv_set = "paint"};
    auto non_unit = buffers.descriptor();
    auto non_unit_tangents = tangents;
    non_unit_tangents[0] = {2.0F, 0.0F, 0.0F, 1.0F};
    non_unit.corner_tangents = non_unit_tangents;
    non_unit.tangent_frame = TangentFrameDescriptor{.uv_set = "paint"};
    auto declaration_without_values = buffers.descriptor();
    declaration_without_values.tangent_frame = TangentFrameDescriptor{.uv_set = "paint"};
    return expect_invalid_argument([&] { static_cast<void>(MeshView(undeclared)); },
                                   "mesh accepted undeclared supplied tangents") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(wrong_count)); },
                                   "mesh accepted an incomplete per-corner tangent buffer") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(non_orthogonal)); },
                                   "mesh accepted a tangent parallel to its normal") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(non_unit)); },
                                   "mesh accepted a non-unit tangent direction") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(declaration_without_values)); },
                                   "mesh accepted a tangent declaration without values");
}

bool derives_total_partitioned_texture_sets() {
    MeshBuffers buffers;
    const MeshView mesh(buffers.descriptor());
    ctex::doc::TextureDocument document;
    const auto ids = document.create_texture_sets_from_mesh(mesh, "lightmap", 2048, 1024, 16);

    return expect(ids.size() == 2, "mesh partitions did not produce two texture sets") &&
           expect(document.texture_set_count() == 2, "document did not retain derived sets") &&
           expect(document.texture_set(ids[0]).descriptor().partition_key == "body",
                  "body partition identity was not preserved") &&
           expect(document.texture_set(ids[1]).descriptor().partition_key == "trim",
                  "trim partition identity was not preserved") &&
           expect(document.texture_set(ids[0]).descriptor().uv_set == "lightmap",
                  "derived texture set lost its second named UV binding") &&
           expect(document.texture_set(ids[0]).descriptor().width == 2048 &&
                      document.texture_set(ids[0]).descriptor().height == 1024 &&
                      document.texture_set(ids[0]).descriptor().default_bit_depth == 16,
                  "derived texture set lost its independent storage description");
}

bool derives_every_partition_source() {
    constexpr std::array kinds{
        PartitionKind::material,
        PartitionKind::object,
        PartitionKind::submesh,
        PartitionKind::explicit_faces,
    };
    constexpr std::array<std::string_view, 4> expected_prefixes{
        "material/",
        "object/",
        "submesh/",
        "faces/",
    };

    for (std::size_t index = 0; index < kinds.size(); ++index) {
        MeshBuffers buffers;
        const std::array partition{MeshPartition{kinds[index], "part", "Part"}};
        buffers.face_partitions = {0, 0};
        auto descriptor = buffers.descriptor();
        descriptor.partitions = partition;

        const MeshView mesh(descriptor);
        ctex::doc::TextureDocument document;
        const auto ids = document.create_texture_sets_from_mesh(mesh, "paint", 512, 512, 8);
        if (!expect(ids.size() == 1 && ids.front().starts_with(expected_prefixes[index]),
                    "partition source was not preserved in texture-set identity")) {
            return false;
        }
    }
    return true;
}

bool refuses_invalid_topology_and_partitioning() {
    MeshBuffers buffers;
    auto missing_assignment = buffers.descriptor();
    missing_assignment.face_partition_indices =
        std::span<const std::uint32_t>(buffers.face_partitions.data(), 1);

    auto invalid_vertex = buffers.descriptor();
    auto bad_indices = buffers.indices;
    bad_indices[5] = 99;
    invalid_vertex.triangle_indices = bad_indices;

    auto missing_partition = buffers.descriptor();
    auto bad_partition_indices = buffers.face_partitions;
    bad_partition_indices[1] = 2;
    missing_partition.face_partition_indices = bad_partition_indices;

    auto missing_material = buffers.descriptor();
    missing_material.face_material_ids = {};

    auto non_finite_vertex = buffers.descriptor();
    auto bad_positions = buffers.positions;
    bad_positions[0].x = std::numeric_limits<float>::infinity();
    non_finite_vertex.positions = bad_positions;

    return expect_invalid_argument([&] { static_cast<void>(MeshView(missing_assignment)); },
                                   "mesh accepted an unassigned face") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(invalid_vertex)); },
                                   "mesh accepted an out-of-range vertex index") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(missing_partition)); },
                                   "mesh accepted a missing partition reference") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(missing_material)); },
                                   "mesh accepted a face without a material identifier") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(non_finite_vertex)); },
                                   "mesh accepted a non-finite vertex position");
}

bool refuses_declared_mesh_limits_before_reading_buffers() {
    const Vec3f vertex{};
    const std::uint32_t index = 0;
    MeshDescriptor too_many_vertices{};
    too_many_vertices.positions =
        std::span<const Vec3f>(&vertex, ctex::mesh::maximum_vertex_count + 1);
    MeshDescriptor too_many_triangles{};
    too_many_triangles.triangle_indices =
        std::span<const std::uint32_t>(&index, (ctex::mesh::maximum_triangle_count + 1) * 3);

    return expect_invalid_argument([&] { static_cast<void>(MeshView(too_many_vertices)); },
                                   "mesh accepted more than the vertex limit") &&
           expect_invalid_argument([&] { static_cast<void>(MeshView(too_many_triangles)); },
                                   "mesh accepted more than the triangle limit");
}

}  // namespace

int main() {
    return accepts_in_memory_attributes_without_modification() &&
                   restores_persisted_revision_and_advances_replacements() &&
                   accepts_declared_supplied_corner_tangents() &&
                   generated_tangents_preserve_mirrored_uv_handedness() &&
                   refuses_undeclared_or_invalid_supplied_tangents() &&
                   derives_total_partitioned_texture_sets() && derives_every_partition_source() &&
                   refuses_invalid_topology_and_partitioning() &&
                   refuses_declared_mesh_limits_before_reading_buffers()
               ? 0
               : 1;
}
