#ifndef CTEX_PICK_RAY_HPP
#define CTEX_PICK_RAY_HPP

#include <array>
#include <cstdint>
#include <ctex/mesh/mesh.hpp>

namespace ctex::pick {

struct Ray {
    mesh::Vec3f origin;
    mesh::Vec3f direction;
};

struct Mat4f {
    // Column-major: element at row r, column c is values[c * 4 + r].
    std::array<float, 16> values;
};

struct ScreenPosition {
    float x;
    float y;
};

struct ViewportSize {
    std::uint32_t width;
    std::uint32_t height;
};

enum class ProjectionKind { perspective, orthographic };

[[nodiscard]] Ray ray_from_screen(ScreenPosition position, ViewportSize viewport, const Mat4f& view,
                                  const Mat4f& projection, ProjectionKind projection_kind);

}  // namespace ctex::pick

#endif
