#include <algorithm>
#include <cmath>
#include <ctex/graph/portable_nodes.hpp>
#include <ctex/paint/blending.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::paint {
namespace {

bool normalized(float value) { return std::isfinite(value) && value >= 0.0F && value <= 1.0F; }

bool valid(graph::ColourValue colour) {
    return normalized(colour.r) && normalized(colour.g) && normalized(colour.b) &&
           normalized(colour.a);
}

std::size_t checked_pixel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        throw std::invalid_argument("stroke blend dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

void validate_colours(std::span<const graph::ColourValue> colours, std::size_t expected,
                      std::string_view role) {
    if (colours.size() != expected ||
        !std::all_of(colours.begin(), colours.end(),
                     [](graph::ColourValue value) { return valid(value); })) {
        throw std::invalid_argument(std::string(role) + " colours are invalid");
    }
}

void validate_strength(std::span<const double> strength, std::size_t expected) {
    if (strength.size() != expected ||
        !std::all_of(strength.begin(), strength.end(), [](double value) {
            return std::isfinite(value) && value >= 0.0 && value <= 1.0;
        })) {
        throw std::invalid_argument("stroke blend strength is invalid");
    }
}

void validate_mode(std::string_view mode) {
    try {
        static_cast<void>(graph::blend_colour(mode, {}, {}, 0.0));
    } catch (const graph::BuiltinNodeEvaluationError&) {
        throw std::invalid_argument("paint blend mode '" + std::string(mode) + "' is invalid");
    }
}

}  // namespace

StrokeSnapshotBlender::StrokeSnapshotBlender(
    std::uint32_t width, std::uint32_t height,
    std::span<const graph::ColourValue> stroke_start_snapshot, std::string_view blend_mode)
    : blend_mode_(blend_mode),
      snapshot_(stroke_start_snapshot.begin(), stroke_start_snapshot.end()),
      result_{.width = width, .height = height, .pixels = snapshot_} {
    const std::size_t pixel_count = checked_pixel_count(width, height);
    validate_colours(snapshot_, pixel_count, "stroke-start snapshot");
    validate_mode(blend_mode_);
}

void StrokeSnapshotBlender::shade(std::span<const graph::ColourValue> paint,
                                  std::span<const double> strength) {
    validate_colours(paint, snapshot_.size(), "paint");
    validate_strength(strength, snapshot_.size());
    std::vector<graph::ColourValue> staged;
    staged.reserve(snapshot_.size());
    for (std::size_t pixel = 0; pixel < snapshot_.size(); ++pixel) {
        staged.push_back(
            graph::blend_colour(blend_mode_, snapshot_[pixel], paint[pixel], strength[pixel]));
    }
    result_.pixels = std::move(staged);
}

void StrokeSnapshotBlender::shade(std::span<const graph::ColourValue> paint,
                                  const DepositionRaster& deposition) {
    if (deposition.width != result_.width || deposition.height != result_.height) {
        throw std::invalid_argument("deposition and stroke snapshot dimensions differ");
    }
    shade(paint, deposition.strength);
}

StrokeBlendRaster blend_stroke_snapshot(std::uint32_t width, std::uint32_t height,
                                        std::span<const graph::ColourValue> stroke_start_snapshot,
                                        std::span<const graph::ColourValue> paint,
                                        const DepositionRaster& deposition,
                                        std::string_view blend_mode) {
    StrokeSnapshotBlender blender(width, height, stroke_start_snapshot, blend_mode);
    blender.shade(paint, deposition);
    return blender.result();
}

}  // namespace ctex::paint
