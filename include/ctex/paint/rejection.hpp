#ifndef CTEX_PAINT_REJECTION_HPP
#define CTEX_PAINT_REJECTION_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/coverage.hpp>
#include <ctex/paint/parameters.hpp>
#include <optional>
#include <span>
#include <vector>

namespace ctex::paint {

inline constexpr double default_alpha_discard_8_bit = 0.1;
inline constexpr double default_alpha_discard_high_precision = 0.004;
inline constexpr ToolParameterDescriptor rejection_depth_bias_parameter{"rejection.depth_bias",
                                                                        1.0e-4, 0.0, 1'000'000.0};
inline constexpr ToolParameterDescriptor rejection_minimum_normal_dot_parameter{
    "rejection.minimum_normal_dot", 0.5, -1.0, 1.0};
inline constexpr double default_depth_rejection_bias = rejection_depth_bias_parameter.default_value;
inline constexpr double default_angle_rejection_dot =
    rejection_minimum_normal_dot_parameter.default_value;

enum class SymmetryDepthPolicy : std::uint8_t {
    require_consistent_per_instance,
    disable_for_derived_symmetry,
};

struct DepthProjectionContext {
    std::uint64_t symmetry_instance{};
    std::uint32_t viewport_width{};
    std::uint32_t viewport_height{};
    std::span<const Vec2d> screen_positions;
    std::span<const double> surface_depth;
    std::span<const double> visible_depth;
    bool transform_consistent{};
};

struct RejectionSettings {
    bool depth_enabled{true};
    double depth_bias{default_depth_rejection_bias};
    SymmetryDepthPolicy symmetry_depth_policy{SymmetryDepthPolicy::require_consistent_per_instance};
    bool angle_enabled{true};
    double minimum_normal_dot{default_angle_rejection_dot};
    bool backface_enabled{};
};

struct RejectionInput {
    std::span<const DepthProjectionContext> depth_contexts;
    // Unit vectors from each surface texel toward the camera.
    std::span<const Vec3d> view_directions;
};

enum class DepthRejectionDisposition : std::uint8_t {
    disabled_by_operation,
    consistent_per_instance,
    disabled_for_derived_symmetry,
};

struct RejectionReport {
    DepthRejectionDisposition depth_disposition{};
    std::size_t depth_rejected_contributions{};
    std::size_t angle_rejected_contributions{};
    std::size_t backface_rejected_texels{};
    RejectionSettings resolved_settings;
    ToolParameterReport parameter_report;
};

struct RejectedStampCoverage {
    std::uint64_t stamp_ordinal{};
    std::vector<double> values;
};

struct RejectedCoverageRaster {
    CoverageRaster coverage;
    std::vector<RejectedStampCoverage> stamp_events;
    RejectionReport report;
};

[[nodiscard]] RejectedCoverageRaster evaluate_rejected_coverage(
    const TextureSpaceRaster& surface, const ResolvedStroke& stroke,
    const RejectionSettings& settings = {}, const RejectionInput& input = {});

enum class AlphaDiscardFormat : std::uint8_t { unorm8, unorm16, floating_point };

struct AlphaDiscardSettings {
    AlphaDiscardFormat format{AlphaDiscardFormat::unorm8};
    std::optional<double> threshold;
};

struct AlphaDiscardResult {
    double threshold{};
    std::vector<double> retained_strength;
    std::vector<std::uint8_t> write_mask;
    ToolParameterReport parameter_report;
};

struct AlphaDiscardThresholdResult {
    double threshold{};
    ToolParameterReport parameter_report;
};

[[nodiscard]] AlphaDiscardThresholdResult alpha_discard_threshold(
    const AlphaDiscardSettings& settings);
[[nodiscard]] AlphaDiscardResult apply_alpha_discard(std::span<const double> accumulated_strength,
                                                     const AlphaDiscardSettings& settings = {});

}  // namespace ctex::paint

#endif
