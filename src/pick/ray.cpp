#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/pick/ray.hpp>
#include <stdexcept>

namespace ctex::pick {
namespace {

using AugmentedMatrix = std::array<std::array<double, 8>, 4>;

struct Vec4d {
    double x;
    double y;
    double z;
    double w;
};

AugmentedMatrix augmented(const Mat4f& matrix) {
    AugmentedMatrix result{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            result[row][column] = matrix.values[column * 4 + row];
        }
        result[row][row + 4] = 1.0;
    }
    return result;
}

std::size_t pivot_row(const AugmentedMatrix& matrix, std::size_t column) {
    std::size_t result = column;
    for (std::size_t row = column + 1; row < 4; ++row) {
        if (std::abs(matrix[row][column]) > std::abs(matrix[result][column])) {
            result = row;
        }
    }
    return result;
}

Mat4f inverse(const Mat4f& matrix) {
    AugmentedMatrix work = augmented(matrix);
    for (std::size_t column = 0; column < 4; ++column) {
        const std::size_t selected = pivot_row(work, column);
        if (std::abs(work[selected][column]) <= 1.0e-12) {
            throw std::invalid_argument("view-projection matrix is singular");
        }
        std::swap(work[column], work[selected]);
        const double pivot = work[column][column];
        for (double& value : work[column]) {
            value /= pivot;
        }
        for (std::size_t row = 0; row < 4; ++row) {
            if (row == column) {
                continue;
            }
            const double factor = work[row][column];
            for (std::size_t entry = 0; entry < 8; ++entry) {
                work[row][entry] -= factor * work[column][entry];
            }
        }
    }

    Mat4f result{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            result.values[column * 4 + row] = static_cast<float>(work[row][column + 4]);
        }
    }
    return result;
}

Mat4f multiply(const Mat4f& left, const Mat4f& right) {
    Mat4f result{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            double value = 0.0;
            for (std::size_t inner = 0; inner < 4; ++inner) {
                value += static_cast<double>(left.values[inner * 4 + row]) *
                         right.values[column * 4 + inner];
            }
            result.values[column * 4 + row] = static_cast<float>(value);
        }
    }
    return result;
}

Vec4d transform(const Mat4f& matrix, Vec4d vector) {
    const std::array input{vector.x, vector.y, vector.z, vector.w};
    std::array<double, 4> output{};
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            output[row] += matrix.values[column * 4 + row] * input[column];
        }
    }
    return {output[0], output[1], output[2], output[3]};
}

mesh::Vec3f cartesian(Vec4d value) {
    if (std::abs(value.w) <= 1.0e-12) {
        throw std::invalid_argument("matrix unprojection produced a point at infinity");
    }
    const mesh::Vec3f result{
        static_cast<float>(value.x / value.w),
        static_cast<float>(value.y / value.w),
        static_cast<float>(value.z / value.w),
    };
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
        throw std::invalid_argument("matrix unprojection produced a non-finite point");
    }
    return result;
}

mesh::Vec3f subtract(mesh::Vec3f left, mesh::Vec3f right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

mesh::Vec3f normalized(mesh::Vec3f value) {
    const double length =
        std::sqrt(static_cast<double>(value.x) * value.x + static_cast<double>(value.y) * value.y +
                  static_cast<double>(value.z) * value.z);
    if (!std::isfinite(length) || length <= 1.0e-12) {
        throw std::invalid_argument("unprojected ray direction is degenerate");
    }
    return {
        static_cast<float>(value.x / length),
        static_cast<float>(value.y / length),
        static_cast<float>(value.z / length),
    };
}

void validate_inputs(ScreenPosition position, ViewportSize viewport) {
    if (viewport.width == 0 || viewport.height == 0) {
        throw std::invalid_argument("ray construction requires a non-empty viewport");
    }
    if (!std::isfinite(position.x) || !std::isfinite(position.y)) {
        throw std::invalid_argument("ray construction requires a finite screen position");
    }
}

}  // namespace

Ray ray_from_screen(ScreenPosition position, ViewportSize viewport, const Mat4f& view,
                    const Mat4f& projection, ProjectionKind projection_kind) {
    validate_inputs(position, viewport);
    const double ndc_x = 2.0 * position.x / viewport.width - 1.0;
    const double ndc_y = 1.0 - 2.0 * position.y / viewport.height;
    const Mat4f inverse_view_projection = inverse(multiply(projection, view));
    const mesh::Vec3f near_point =
        cartesian(transform(inverse_view_projection, {ndc_x, ndc_y, -1.0, 1.0}));
    const mesh::Vec3f far_point =
        cartesian(transform(inverse_view_projection, {ndc_x, ndc_y, 1.0, 1.0}));

    if (projection_kind == ProjectionKind::orthographic) {
        return {near_point, normalized(subtract(far_point, near_point))};
    }
    const mesh::Vec3f camera = cartesian(transform(inverse(view), {0.0, 0.0, 0.0, 1.0}));
    return {camera, normalized(subtract(far_point, camera))};
}

}  // namespace ctex::pick
