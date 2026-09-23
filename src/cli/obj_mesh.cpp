#include "obj_mesh.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ctex::cli {
namespace {

struct Corner {
    std::int64_t position{};
    std::int64_t uv{};
    std::int64_t normal{};
    friend bool operator<(Corner left, Corner right) noexcept {
        return std::tie(left.position, left.uv, left.normal) <
               std::tie(right.position, right.uv, right.normal);
    }
};

std::string_view trim(std::string_view value) {
    const std::size_t first = value.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

std::pair<std::string_view, std::string_view> split_first(std::string_view value) {
    value = trim(value);
    const std::size_t separator = value.find_first_of(" \t");
    return separator == std::string_view::npos
               ? std::pair{value, std::string_view{}}
               : std::pair{value.substr(0, separator), trim(value.substr(separator + 1))};
}

template <typename Value>
Value number(std::string_view text, std::string_view field) {
    Value value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::invalid_argument("OBJ has an invalid " + std::string(field));
    }
    if constexpr (std::is_floating_point_v<Value>) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument("OBJ has a non-finite " + std::string(field));
        }
    }
    return value;
}

std::vector<std::string_view> words(std::string_view value) {
    std::vector<std::string_view> result;
    while (!(value = trim(value)).empty()) {
        const auto [word, remainder] = split_first(value);
        result.push_back(word);
        value = remainder;
    }
    return result;
}

Corner corner(std::string_view token) {
    Corner result;
    const std::size_t first = token.find('/');
    if (first == std::string_view::npos) {
        result.position = number<std::int64_t>(token, "face position index");
        return result;
    }
    result.position = number<std::int64_t>(token.substr(0, first), "face position index");
    const std::size_t second = token.find('/', first + 1);
    const std::string_view uv = token.substr(
        first + 1, second == std::string_view::npos ? token.size() : second - first - 1);
    if (!uv.empty()) result.uv = number<std::int64_t>(uv, "face UV index");
    if (second != std::string_view::npos && second + 1 < token.size()) {
        result.normal = number<std::int64_t>(token.substr(second + 1), "face normal index");
    }
    return result;
}

std::size_t resolved_index(std::int64_t index, std::size_t count, std::string_view field) {
    if (index == 0) throw std::invalid_argument("OBJ " + std::string(field) + " index is zero");
    const std::int64_t resolved = index > 0 ? index - 1 : static_cast<std::int64_t>(count) + index;
    if (resolved < 0 || static_cast<std::uint64_t>(resolved) >= count) {
        throw std::invalid_argument("OBJ " + std::string(field) + " index is out of range");
    }
    return static_cast<std::size_t>(resolved);
}

mesh::Vec3f subtract(mesh::Vec3f left, mesh::Vec3f right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

mesh::Vec3f cross(mesh::Vec3f left, mesh::Vec3f right) {
    return {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

void add(mesh::Vec3f& destination, mesh::Vec3f value) {
    destination.x += value.x;
    destination.y += value.y;
    destination.z += value.z;
}

mesh::Vec3f normalized(mesh::Vec3f value) {
    const double length =
        std::sqrt(static_cast<double>(value.x) * value.x + static_cast<double>(value.y) * value.y +
                  static_cast<double>(value.z) * value.z);
    if (!std::isfinite(length) || length <= 1.0e-20) return {0.0F, 0.0F, 1.0F};
    return {static_cast<float>(value.x / length), static_cast<float>(value.y / length),
            static_cast<float>(value.z / length)};
}

struct Parser {
    std::size_t maximum_vertices{};
    std::size_t maximum_triangles{};
    std::vector<mesh::Vec3f> source_positions;
    std::vector<mesh::Vec3f> source_normals;
    std::vector<mesh::Vec2f> source_uv;
    ObjMesh result;
    std::map<Corner, std::uint32_t> vertices;
    std::vector<bool> generated_normal;
    std::uint32_t current_material{};

    Parser(std::size_t vertices_limit, std::size_t triangles_limit)
        : maximum_vertices(vertices_limit), maximum_triangles(triangles_limit) {
        result.material_names.push_back("default");
    }

    void check_source_limit(std::size_t count, std::string_view field) const {
        if (count > maximum_vertices) {
            throw std::invalid_argument("OBJ " + std::string(field) + " exceeds configured limit");
        }
    }

    std::uint32_t material(std::string_view name) {
        if (name.empty()) throw std::invalid_argument("OBJ material name is empty");
        const auto found = std::ranges::find(result.material_names, name);
        if (found != result.material_names.end()) {
            return static_cast<std::uint32_t>(found - result.material_names.begin());
        }
        result.material_names.emplace_back(name);
        return static_cast<std::uint32_t>(result.material_names.size() - 1);
    }

    std::uint32_t vertex(Corner value) {
        const std::size_t position =
            resolved_index(value.position, source_positions.size(), "position");
        if (value.uv == 0) throw std::invalid_argument("OBJ face has no UV coordinate");
        const std::size_t uv = resolved_index(value.uv, source_uv.size(), "UV");
        const std::size_t normal =
            value.normal == 0 ? 0 : resolved_index(value.normal, source_normals.size(), "normal");
        value = {.position = static_cast<std::int64_t>(position),
                 .uv = static_cast<std::int64_t>(uv),
                 .normal = value.normal == 0 ? -1 : static_cast<std::int64_t>(normal)};
        if (const auto found = vertices.find(value); found != vertices.end()) return found->second;
        if (result.positions.size() >= maximum_vertices ||
            result.positions.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("OBJ remapped vertices exceed configured limit");
        }
        const auto index = static_cast<std::uint32_t>(result.positions.size());
        vertices.emplace(value, index);
        result.positions.push_back(source_positions[position]);
        result.uv.push_back(source_uv[uv]);
        result.normals.push_back(value.normal < 0 ? mesh::Vec3f{} : source_normals[normal]);
        generated_normal.push_back(value.normal < 0);
        return index;
    }

    void face(std::string_view arguments) {
        const std::vector<std::string_view> tokens = words(arguments);
        if (tokens.size() < 3) throw std::invalid_argument("OBJ face has fewer than three corners");
        std::vector<std::uint32_t> polygon;
        polygon.reserve(tokens.size());
        for (const std::string_view token : tokens) polygon.push_back(vertex(corner(token)));
        const std::size_t added_triangles = polygon.size() - 2;
        if (added_triangles >
            maximum_triangles - std::min(maximum_triangles, result.face_partitions.size())) {
            throw std::invalid_argument("OBJ triangles exceed configured limit");
        }
        for (std::size_t index = 1; index + 1 < polygon.size(); ++index) {
            result.triangle_indices.insert(result.triangle_indices.end(),
                                           {polygon[0], polygon[index], polygon[index + 1]});
            result.face_partitions.push_back(current_material);
            result.face_materials.push_back(current_material);
        }
    }

    void line(std::string_view value) {
        value = trim(value.substr(0, value.find('#')));
        if (value.empty()) return;
        const auto [keyword, arguments] = split_first(value);
        const std::vector<std::string_view> values = words(arguments);
        if (keyword == "v") {
            if (values.size() < 3) throw std::invalid_argument("OBJ position is incomplete");
            source_positions.push_back({number<float>(values[0], "position"),
                                        number<float>(values[1], "position"),
                                        number<float>(values[2], "position")});
            check_source_limit(source_positions.size(), "positions");
        } else if (keyword == "vn") {
            if (values.size() < 3) throw std::invalid_argument("OBJ normal is incomplete");
            source_normals.push_back({number<float>(values[0], "normal"),
                                      number<float>(values[1], "normal"),
                                      number<float>(values[2], "normal")});
            check_source_limit(source_normals.size(), "normals");
        } else if (keyword == "vt") {
            if (values.size() < 2) throw std::invalid_argument("OBJ UV is incomplete");
            source_uv.push_back({number<float>(values[0], "UV"), number<float>(values[1], "UV")});
            check_source_limit(source_uv.size(), "UV coordinates");
        } else if (keyword == "usemtl") {
            current_material = material(arguments);
        } else if (keyword == "f") {
            face(arguments);
        }
    }

    ObjMesh finish() {
        if (result.triangle_indices.empty()) throw std::invalid_argument("OBJ has no triangles");
        for (std::size_t triangle = 0; triangle < result.triangle_indices.size(); triangle += 3) {
            const std::uint32_t first = result.triangle_indices[triangle];
            const std::uint32_t second = result.triangle_indices[triangle + 1];
            const std::uint32_t third = result.triangle_indices[triangle + 2];
            const mesh::Vec3f face_normal =
                cross(subtract(result.positions[second], result.positions[first]),
                      subtract(result.positions[third], result.positions[first]));
            for (const std::uint32_t index : {first, second, third}) {
                if (generated_normal[index]) add(result.normals[index], face_normal);
            }
        }
        for (std::size_t index = 0; index < result.normals.size(); ++index) {
            result.normals[index] = normalized(result.normals[index]);
        }
        return std::move(result);
    }
};

}  // namespace

mesh::MeshDescriptor ObjMesh::descriptor() const {
    uv_sets = {{"uv0", uv}};
    partitions.clear();
    partitions.reserve(material_names.size());
    for (const std::string& name : material_names) {
        partitions.push_back({mesh::PartitionKind::material, name, name});
    }
    return {.positions = positions,
            .normals = normals,
            .vertex_colors = {},
            .triangle_indices = triangle_indices,
            .uv_sets = uv_sets,
            .default_uv_set = "uv0",
            .partitions = partitions,
            .face_partition_indices = face_partitions,
            .face_material_ids = face_materials};
}

ObjMesh parse_obj_mesh(std::span<const std::byte> bytes, std::size_t maximum_vertices,
                       std::size_t maximum_triangles) {
    if (maximum_vertices == 0 || maximum_triangles == 0) {
        throw std::invalid_argument("OBJ limits must be non-zero");
    }
    Parser parser(maximum_vertices, maximum_triangles);
    const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::size_t offset{};
    while (offset < text.size()) {
        const std::size_t end = text.find('\n', offset);
        parser.line(text.substr(
            offset, end == std::string_view::npos ? text.size() - offset : end - offset));
        if (end == std::string_view::npos) break;
        offset = end + 1;
    }
    return parser.finish();
}

}  // namespace ctex::cli
