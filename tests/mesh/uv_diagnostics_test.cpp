#include <array>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>
#include <ctex/mesh/uv_diagnostics.hpp>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using ctex::mesh::MeshDescriptor;
using ctex::mesh::MeshPartition;
using ctex::mesh::MeshView;
using ctex::mesh::PartitionKind;
using ctex::mesh::UvSetView;
using ctex::mesh::Vec2f;
using ctex::mesh::Vec3f;

struct DiagnosticMesh {
    std::array<Vec3f, 6> positions{};
    std::array<Vec3f, 6> normals{};
    std::array<std::uint32_t, 6> indices{0, 1, 2, 3, 4, 5};
    std::array<Vec2f, 6> uv{};
    std::array<UvSetView, 1> uv_sets{};
    std::array<MeshPartition, 2> partitions{
        MeshPartition{PartitionKind::material, "body", "Body"},
        MeshPartition{PartitionKind::material, "trim", "Trim"},
    };
    std::array<std::uint32_t, 2> face_partitions{0, 0};
    std::array<std::uint32_t, 2> face_materials{1, 1};

    DiagnosticMesh() {
        normals.fill({0.0F, 0.0F, 1.0F});
        uv_sets = {UvSetView{"paint", uv}};
    }

    [[nodiscard]] MeshDescriptor descriptor() const {
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

bool reports_positive_area_overlap() {
    DiagnosticMesh source;
    source.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.1F, 0.1F}, Vec2f{0.7F, 0.1F}, Vec2f{0.1F, 0.7F},
    };
    const MeshView mesh(source.descriptor());
    const ctex::mesh::UvOverlapReport report = ctex::mesh::analyze_uv_overlaps(mesh, "paint", 0);

    DiagnosticMesh crossing;
    crossing.uv = {
        Vec2f{0.0F, 0.0F},  Vec2f{1.0F, 0.0F},  Vec2f{0.5F, 1.0F},
        Vec2f{0.0F, 0.75F}, Vec2f{1.0F, 0.75F}, Vec2f{0.5F, -0.25F},
    };
    const MeshView crossing_mesh(crossing.descriptor());
    const auto crossing_report = ctex::mesh::analyze_uv_overlaps(crossing_mesh, "paint", 0);
    return expect(report.face_indices == std::vector<std::uint32_t>{0, 1},
                  "overlap report omitted affected faces") &&
           expect(report.overlap_pair_count == 1 && report.candidate_pair_count == 1,
                  "overlap report omitted deterministic work counts") &&
           expect(crossing_report.face_indices == std::vector<std::uint32_t>{0, 1} &&
                      crossing_report.overlap_pair_count == 1,
                  "crossing opposite-winding triangles were not clipped to their overlap");
}

bool ignores_boundaries_degenerates_and_other_partitions() {
    DiagnosticMesh boundary;
    boundary.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{1.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 1.0F}, Vec2f{0.0F, 1.0F},
    };
    const MeshView boundary_mesh(boundary.descriptor());
    const auto boundary_report = ctex::mesh::analyze_uv_overlaps(boundary_mesh, "paint", 0);

    DiagnosticMesh partitioned;
    partitioned.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
    };
    partitioned.face_partitions = {0, 1};
    const MeshView partitioned_mesh(partitioned.descriptor());
    const auto body_report = ctex::mesh::analyze_uv_overlaps(partitioned_mesh, "paint", 0);
    const auto trim_report = ctex::mesh::analyze_uv_overlaps(partitioned_mesh, "paint", 1);

    DiagnosticMesh degenerate;
    degenerate.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.1F, 0.1F}, Vec2f{0.2F, 0.2F}, Vec2f{0.3F, 0.3F},
    };
    const MeshView degenerate_mesh(degenerate.descriptor());
    const auto degenerate_report = ctex::mesh::analyze_uv_overlaps(degenerate_mesh, "paint", 0);

    return expect(boundary_report.face_indices.empty() && boundary_report.overlap_pair_count == 0,
                  "shared UV boundary was reported as overlap") &&
           expect(body_report.face_indices.empty() && trim_report.face_indices.empty(),
                  "faces from different texture-set partitions were compared") &&
           expect(degenerate_report.face_indices.empty(),
                  "degenerate UV triangle was reported as positive-area overlap");
}

bool rejects_missing_inputs() {
    DiagnosticMesh source;
    source.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
    };
    const MeshView mesh(source.descriptor());
    bool missing_uv = false;
    bool missing_partition = false;
    try {
        static_cast<void>(ctex::mesh::analyze_uv_overlaps(mesh, "missing", 0));
    } catch (const std::out_of_range&) {
        missing_uv = true;
    }
    try {
        static_cast<void>(ctex::mesh::analyze_uv_overlaps(mesh, "paint", 2));
    } catch (const std::out_of_range&) {
        missing_partition = true;
    }
    return expect(missing_uv && missing_partition,
                  "UV diagnostics accepted a missing UV set or partition");
}

bool reports_non_udim_coverage_and_outside_faces() {
    DiagnosticMesh source;
    source.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{0.5F, 0.0F}, Vec2f{0.0F, 0.5F},
        Vec2f{1.1F, 0.0F}, Vec2f{1.2F, 0.0F}, Vec2f{1.1F, 0.1F},
    };
    const MeshView mesh(source.descriptor());
    const ctex::mesh::UvCoverageReport report =
        ctex::mesh::analyze_uv_coverage(mesh, "paint", 0, {.width = 4, .height = 4});
    return expect(report.selected_face_count == 2,
                  "coverage report omitted selected partition faces") &&
           expect(report.covered_sample_count == 3 && report.uncovered_sample_count == 13 &&
                      report.uncovered_fraction == 13.0 / 16.0,
                  "coverage report did not measure uncovered unit-square texels") &&
           expect(report.outside_face_indices == std::vector<std::uint32_t>{1},
                  "coverage report omitted the out-of-range face") &&
           expect(report.tested_sample_count >= report.covered_sample_count,
                  "coverage report omitted sample-test work");
}

bool coverage_is_partition_scoped_and_bounded() {
    DiagnosticMesh source;
    source.uv = {
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
        Vec2f{0.0F, 0.0F}, Vec2f{1.0F, 0.0F}, Vec2f{0.0F, 1.0F},
    };
    source.face_partitions = {0, 1};
    const MeshView mesh(source.descriptor());
    const auto body = ctex::mesh::analyze_uv_coverage(mesh, "paint", 0, {.width = 4, .height = 4});
    const auto trim = ctex::mesh::analyze_uv_coverage(mesh, "paint", 1, {.width = 4, .height = 4});
    bool zero_refused = false;
    bool ceiling_refused = false;
    try {
        static_cast<void>(
            ctex::mesh::analyze_uv_coverage(mesh, "paint", 0, {.width = 0, .height = 4}));
    } catch (const std::invalid_argument&) {
        zero_refused = true;
    }
    try {
        static_cast<void>(
            ctex::mesh::analyze_uv_coverage(mesh, "paint", 0, {.width = 16'385, .height = 16'384}));
    } catch (const std::length_error&) {
        ceiling_refused = true;
    }
    return expect(body.selected_face_count == 1 && trim.selected_face_count == 1 &&
                      body.covered_sample_count == 10 && trim.covered_sample_count == 10,
                  "coverage mixed faces between texture-set partitions") &&
           expect(zero_refused && ceiling_refused,
                  "coverage accepted zero dimensions or an excessive sample count");
}

}  // namespace

int main() {
    return reports_positive_area_overlap() &&
                   ignores_boundaries_degenerates_and_other_partitions() &&
                   rejects_missing_inputs() && reports_non_udim_coverage_and_outside_faces() &&
                   coverage_is_partition_scoped_and_bounded()
               ? 0
               : 1;
}
