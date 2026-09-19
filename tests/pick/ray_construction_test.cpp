#include <cmath>
#include <ctex/pick/ray.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

constexpr ctex::pick::Mat4f identity{
    {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F,
     1.0F},
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool close(float left, float right, float tolerance = 1.0e-5F) {
    return std::abs(left - right) <= tolerance;
}

bool same_vector(ctex::mesh::Vec3f value, ctex::mesh::Vec3f expected) {
    return close(value.x, expected.x) && close(value.y, expected.y) && close(value.z, expected.z);
}

ctex::pick::Mat4f perspective_90_degrees(float near_distance, float far_distance) {
    const float depth = near_distance - far_distance;
    return {{
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        (far_distance + near_distance) / depth,
        -1.0F,
        0.0F,
        0.0F,
        (2.0F * far_distance * near_distance) / depth,
        0.0F,
    }};
}

ctex::pick::Mat4f orthographic(float near_distance, float far_distance) {
    const float depth = near_distance - far_distance;
    auto result = identity;
    result.values[10] = 2.0F / depth;
    result.values[14] = (far_distance + near_distance) / depth;
    return result;
}

bool constructs_perspective_rays_from_the_camera() {
    const auto projection = perspective_90_degrees(1.0F, 10.0F);
    const auto center = ctex::pick::ray_from_screen(
        {50.0F, 50.0F}, {100, 100}, identity, projection, ctex::pick::ProjectionKind::perspective);
    const auto top_left = ctex::pick::ray_from_screen(
        {0.0F, 0.0F}, {100, 100}, identity, projection, ctex::pick::ProjectionKind::perspective);
    const float inverse_root_three = 1.0F / std::sqrt(3.0F);

    return expect(same_vector(center.origin, {0.0F, 0.0F, 0.0F}),
                  "perspective ray did not start at the camera") &&
           expect(same_vector(center.direction, {0.0F, 0.0F, -1.0F}),
                  "center perspective ray used the wrong view direction") &&
           expect(same_vector(top_left.direction,
                              {-inverse_root_three, inverse_root_three, -inverse_root_three}),
                  "top-left screen coordinate used the wrong NDC convention");
}

bool constructs_parallel_orthographic_rays_on_the_near_plane() {
    const auto projection = orthographic(1.0F, 10.0F);
    const auto center = ctex::pick::ray_from_screen(
        {50.0F, 50.0F}, {100, 100}, identity, projection, ctex::pick::ProjectionKind::orthographic);
    const auto top_left = ctex::pick::ray_from_screen(
        {0.0F, 0.0F}, {100, 100}, identity, projection, ctex::pick::ProjectionKind::orthographic);

    return expect(same_vector(center.origin, {0.0F, 0.0F, -1.0F}),
                  "orthographic center ray did not start on the near plane") &&
           expect(same_vector(top_left.origin, {-1.0F, 1.0F, -1.0F}),
                  "orthographic screen position did not move the ray origin") &&
           expect(same_vector(center.direction, top_left.direction) &&
                      same_vector(center.direction, {0.0F, 0.0F, -1.0F}),
                  "orthographic rays were not parallel to the view direction");
}

bool applies_the_view_transform() {
    auto view = identity;
    view.values[12] = -2.0F;
    view.values[13] = -3.0F;
    view.values[14] = -4.0F;
    const auto ray = ctex::pick::ray_from_screen({50.0F, 50.0F}, {100, 100}, view,
                                                 perspective_90_degrees(1.0F, 10.0F),
                                                 ctex::pick::ProjectionKind::perspective);
    return expect(same_vector(ray.origin, {2.0F, 3.0F, 4.0F}),
                  "view inversion did not recover the world-space camera") &&
           expect(same_vector(ray.direction, {0.0F, 0.0F, -1.0F}),
                  "translated view changed the camera direction");
}

bool rejects_singular_input() {
    const ctex::pick::Mat4f singular{};
    try {
        static_cast<void>(ctex::pick::ray_from_screen({0.0F, 0.0F}, {100, 100}, identity, singular,
                                                      ctex::pick::ProjectionKind::perspective));
    } catch (const std::invalid_argument&) {
        return true;
    }
    return expect(false, "singular projection matrix was accepted");
}

}  // namespace

int main() {
    return constructs_perspective_rays_from_the_camera() &&
                   constructs_parallel_orthographic_rays_on_the_near_plane() &&
                   applies_the_view_transform() && rejects_singular_input()
               ? 0
               : 1;
}
