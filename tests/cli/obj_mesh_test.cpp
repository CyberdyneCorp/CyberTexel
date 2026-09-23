#include "../../src/cli/obj_mesh.hpp"

#include <cstddef>
#include <ctex/mesh/mesh.hpp>
#include <iostream>
#include <span>
#include <string_view>

namespace {

std::span<const std::byte> bytes(std::string_view text) {
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

bool polygon_negative_indices_and_generated_normals() {
    constexpr std::string_view source =
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
        "usemtl body\nf -4/-4 -3/-3 -2/-2 -1/-1\n";
    ctex::cli::ObjMesh mesh = ctex::cli::parse_obj_mesh(bytes(source), 16, 4);
    const ctex::mesh::MeshView validated(mesh.descriptor());
    return expect(validated.attributes().vertex_count == 4 && validated.triangle_count() == 2,
                  "OBJ polygon did not triangulate") &&
           expect(mesh.material_names.size() == 2 && mesh.material_names[1] == "body" &&
                      mesh.face_partitions.size() == 2,
                  "OBJ material partitions changed") &&
           expect(mesh.normals.front() == ctex::mesh::Vec3f{0.0F, 0.0F, 1.0F},
                  "OBJ generated normal is incorrect");
}

bool limits_and_missing_uv_are_refused() {
    constexpr std::string_view limited =
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n";
    bool over_limit{};
    bool missing_uv{};
    try {
        static_cast<void>(ctex::cli::parse_obj_mesh(bytes(limited), 2, 1));
    } catch (const std::invalid_argument&) {
        over_limit = true;
    }
    try {
        static_cast<void>(
            ctex::cli::parse_obj_mesh(bytes("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n"), 3, 1));
    } catch (const std::invalid_argument&) {
        missing_uv = true;
    }
    return expect(over_limit, "OBJ vertex limit was ignored") &&
           expect(missing_uv, "OBJ face without UVs was accepted");
}

bool portable_float_parser_accepts_scientific_and_rejects_invalid_values() {
    constexpr std::string_view scientific =
        "v 0 0 0\nv 1.25e0 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n";
    const ctex::cli::ObjMesh mesh = ctex::cli::parse_obj_mesh(bytes(scientific), 3, 1);
    bool suffix_refused{};
    bool non_finite_refused{};
    try {
        static_cast<void>(ctex::cli::parse_obj_mesh(
            bytes("v 0 0 0\nv 1.0x 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n"), 3, 1));
    } catch (const std::invalid_argument&) {
        suffix_refused = true;
    }
    try {
        static_cast<void>(ctex::cli::parse_obj_mesh(
            bytes("v 0 0 0\nv nan 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n"), 3, 1));
    } catch (const std::invalid_argument&) {
        non_finite_refused = true;
    }
    return expect(mesh.positions[1].x == 1.25F, "OBJ scientific float changed") &&
           expect(suffix_refused, "OBJ float suffix was accepted") &&
           expect(non_finite_refused, "OBJ non-finite float was accepted");
}

}  // namespace

int main() {
    return polygon_negative_indices_and_generated_normals() &&
                   limits_and_missing_uv_are_refused() &&
                   portable_float_parser_accepts_scientific_and_rejects_invalid_values()
               ? 0
               : 1;
}
