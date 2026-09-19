#ifndef CTEX_PAINT_STROKE_HPP
#define CTEX_PAINT_STROKE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/parameters.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::paint {

inline constexpr std::uint32_t canonical_stroke_reconstruction_version = 1;
inline constexpr std::uint64_t stabilization_time_step_nanoseconds = 1'000'000;
inline constexpr double stroke_position_tolerance = 1.0e-6;
inline constexpr double default_spacing_fraction = 0.1;
inline constexpr double minimum_spacing_fraction = 0.01;
inline constexpr double maximum_spacing_fraction = 4.0;
inline constexpr double default_stroke_radius = 1.0;
inline constexpr double minimum_stroke_radius = stroke_position_tolerance;
inline constexpr double maximum_stroke_radius = 1'000'000.0;
inline constexpr double maximum_stroke_rotation_radians = 6.28318530717958647692;
inline constexpr ToolParameterDescriptor stroke_radius_parameter{
    "stroke.radius", default_stroke_radius, minimum_stroke_radius, maximum_stroke_radius};
inline constexpr ToolParameterDescriptor stroke_spacing_parameter{
    "stroke.spacing_fraction", default_spacing_fraction, minimum_spacing_fraction,
    maximum_spacing_fraction};
inline constexpr ToolParameterDescriptor stroke_opacity_parameter{"stroke.opacity", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_hardness_parameter{"stroke.hardness", 1.0, 0.0,
                                                                   1.0};
inline constexpr ToolParameterDescriptor stroke_rotation_parameter{"stroke.rotation_radians", 0.0,
                                                                   -maximum_stroke_rotation_radians,
                                                                   maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor stroke_elongation_parameter{"stroke.elongation", 1.0, 0.01,
                                                                     100.0};
inline constexpr ToolParameterDescriptor stroke_flow_parameter{"stroke.flow", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_stabilizer_radius_parameter{
    "stroke.stabilizer.radius", 0.0, 0.0, maximum_stroke_radius};
inline constexpr ToolParameterDescriptor stroke_stabilizer_time_parameter{
    "stroke.stabilizer.time_constant_seconds", 0.0, 0.0, 60.0};
inline constexpr ToolParameterDescriptor stroke_jitter_position_parameter{
    "stroke.jitter.position_fraction", 0.0, 0.0, 4.0};
inline constexpr ToolParameterDescriptor stroke_jitter_radius_parameter{
    "stroke.jitter.radius_fraction", 0.0, 0.0, 0.99};
inline constexpr ToolParameterDescriptor stroke_jitter_rotation_parameter{
    "stroke.jitter.rotation_radians", 0.0, 0.0, maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor stroke_jitter_opacity_parameter{"stroke.jitter.opacity",
                                                                         0.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_jitter_flow_parameter{"stroke.jitter.flow", 0.0,
                                                                      0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_taper_floor_parameter{"stroke.taper.floor", 0.0,
                                                                      0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_grid_step_parameter{
    "stroke.constraint.grid_step", 1.0, stroke_position_tolerance, maximum_stroke_radius};
inline constexpr std::uint32_t maximum_radial_symmetry_count = 4'096;
inline constexpr ToolParameterDescriptor stroke_radial_count_parameter{
    "stroke.symmetry.radial_count", 1.0, 1.0, maximum_radial_symmetry_count};
inline constexpr ToolParameterDescriptor stroke_pressure_radius_minimum_parameter{
    "stroke.input.pressure_radius.minimum_output", 0.01, 0.01, 100.0};
inline constexpr ToolParameterDescriptor stroke_pressure_radius_maximum_parameter{
    "stroke.input.pressure_radius.maximum_output", 1.0, 0.01, 100.0};
inline constexpr ToolParameterDescriptor stroke_pressure_opacity_minimum_parameter{
    "stroke.input.pressure_opacity.minimum_output", 0.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_opacity_maximum_parameter{
    "stroke.input.pressure_opacity.maximum_output", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_hardness_minimum_parameter{
    "stroke.input.pressure_hardness.minimum_output", 0.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_hardness_maximum_parameter{
    "stroke.input.pressure_hardness.maximum_output", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_flow_minimum_parameter{
    "stroke.input.pressure_flow.minimum_output", 0.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_flow_maximum_parameter{
    "stroke.input.pressure_flow.maximum_output", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_pressure_rotation_minimum_parameter{
    "stroke.input.pressure_rotation.minimum_output", 0.0, -maximum_stroke_rotation_radians,
    maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor stroke_pressure_rotation_maximum_parameter{
    "stroke.input.pressure_rotation.maximum_output", 1.0, -maximum_stroke_rotation_radians,
    maximum_stroke_rotation_radians};
inline constexpr ToolParameterDescriptor stroke_tilt_rotation_minimum_parameter{
    "stroke.input.tilt_rotation.minimum_output", 0.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_tilt_rotation_maximum_parameter{
    "stroke.input.tilt_rotation.maximum_output", 1.0, 0.0, 1.0};
inline constexpr ToolParameterDescriptor stroke_tilt_elongation_minimum_parameter{
    "stroke.input.tilt_elongation.minimum_output", 1.0, 0.01, 100.0};
inline constexpr ToolParameterDescriptor stroke_tilt_elongation_maximum_parameter{
    "stroke.input.tilt_elongation.maximum_output", 2.0, 0.01, 100.0};

struct Vec3d {
    double x{};
    double y{};
    double z{};
    friend constexpr bool operator==(Vec3d, Vec3d) noexcept = default;
};

struct Vec2d {
    double x{};
    double y{};
    friend constexpr bool operator==(Vec2d, Vec2d) noexcept = default;
};

struct StrokeFrame {
    Vec3d tangent{1.0, 0.0, 0.0};
    Vec3d bitangent{0.0, 1.0, 0.0};
    Vec3d normal{0.0, 0.0, 1.0};
    friend constexpr bool operator==(StrokeFrame, StrokeFrame) noexcept = default;
};

struct StrokeInputSample {
    Vec3d position;
    StrokeFrame frame;
    std::uint64_t timestamp_nanoseconds{};
    std::optional<double> pressure;
    Vec2d tilt;
    friend constexpr bool operator==(const StrokeInputSample&,
                                     const StrokeInputSample&) noexcept = default;
};

enum class TipMode : std::uint8_t { continuous_sweep, discrete_alpha };

struct StabilizerSettings {
    double radius{};
    double time_constant_seconds{};
    friend constexpr bool operator==(StabilizerSettings, StabilizerSettings) noexcept = default;
};

struct ResponseCurvePoint {
    double input{};
    double output{};
    friend constexpr bool operator==(ResponseCurvePoint, ResponseCurvePoint) noexcept = default;
};

struct ResponseCurve {
    std::vector<ResponseCurvePoint> points{{0.0, 0.0}, {1.0, 1.0}};
    friend bool operator==(const ResponseCurve&, const ResponseCurve&) = default;
};

struct ResponseMapping {
    bool enabled{};
    ResponseCurve curve;
    double minimum_output{};
    double maximum_output{1.0};
    friend bool operator==(const ResponseMapping&, const ResponseMapping&) = default;
};

struct StrokeInputMapping {
    ResponseMapping pressure_radius{
        .enabled = true,
        .curve = {},
        .minimum_output = stroke_pressure_radius_minimum_parameter.default_value,
        .maximum_output = stroke_pressure_radius_maximum_parameter.default_value};
    ResponseMapping pressure_opacity{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_pressure_opacity_minimum_parameter.default_value,
        .maximum_output = stroke_pressure_opacity_maximum_parameter.default_value};
    ResponseMapping pressure_hardness{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_pressure_hardness_minimum_parameter.default_value,
        .maximum_output = stroke_pressure_hardness_maximum_parameter.default_value};
    ResponseMapping pressure_flow{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_pressure_flow_minimum_parameter.default_value,
        .maximum_output = stroke_pressure_flow_maximum_parameter.default_value};
    ResponseMapping pressure_rotation{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_pressure_rotation_minimum_parameter.default_value,
        .maximum_output = stroke_pressure_rotation_maximum_parameter.default_value};
    ResponseMapping tilt_rotation{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_tilt_rotation_minimum_parameter.default_value,
        .maximum_output = stroke_tilt_rotation_maximum_parameter.default_value};
    ResponseMapping tilt_elongation{
        .enabled = false,
        .curve = {},
        .minimum_output = stroke_tilt_elongation_minimum_parameter.default_value,
        .maximum_output = stroke_tilt_elongation_maximum_parameter.default_value};
    friend bool operator==(const StrokeInputMapping&, const StrokeInputMapping&) = default;
};

struct JitterSettings {
    std::uint64_t seed{};
    double position_fraction{stroke_jitter_position_parameter.default_value};
    double radius_fraction{stroke_jitter_radius_parameter.default_value};
    double rotation_radians{stroke_jitter_rotation_parameter.default_value};
    double opacity{stroke_jitter_opacity_parameter.default_value};
    double flow{stroke_jitter_flow_parameter.default_value};
    friend constexpr bool operator==(JitterSettings, JitterSettings) noexcept = default;
};

enum class TaperUnit : std::uint8_t { none, stamp_count, distance };

struct TaperSpan {
    TaperUnit unit{TaperUnit::none};
    double extent{};
    friend constexpr bool operator==(TaperSpan, TaperSpan) noexcept = default;
};

struct TaperSettings {
    TaperSpan entry;
    TaperSpan exit;
    double floor{stroke_taper_floor_parameter.default_value};
    bool affect_radius{true};
    bool affect_opacity{true};
    friend constexpr bool operator==(TaperSettings, TaperSettings) noexcept = default;
};

enum class ConstraintMode : std::uint8_t { none, straight_line, dominant_axis, grid };

struct ConstraintSettings {
    ConstraintMode mode{ConstraintMode::none};
    double grid_step{stroke_grid_step_parameter.default_value};
    friend constexpr bool operator==(ConstraintSettings, ConstraintSettings) noexcept = default;
};

enum class SymmetryAxis : std::uint8_t { x, y, z };

struct SymmetrySettings {
    bool mirror_x{};
    bool mirror_y{};
    bool mirror_z{};
    std::uint32_t radial_count{
        static_cast<std::uint32_t>(stroke_radial_count_parameter.default_value)};
    SymmetryAxis radial_axis{SymmetryAxis::z};
    friend constexpr bool operator==(SymmetrySettings, SymmetrySettings) noexcept = default;
};

struct StrokeSettings {
    std::uint32_t reconstruction_version{canonical_stroke_reconstruction_version};
    TipMode tip_mode{TipMode::continuous_sweep};
    double spacing_fraction{default_spacing_fraction};
    double radius{default_stroke_radius};
    double opacity{stroke_opacity_parameter.default_value};
    double hardness{stroke_hardness_parameter.default_value};
    double rotation_radians{stroke_rotation_parameter.default_value};
    double elongation{stroke_elongation_parameter.default_value};
    double flow{stroke_flow_parameter.default_value};
    std::string tip_resource_identity{"builtin.circle"};
    StabilizerSettings stabilizer;
    StrokeInputMapping input_mapping;
    JitterSettings jitter;
    TaperSettings taper;
    ConstraintSettings constraint;
    SymmetrySettings symmetry;
    friend bool operator==(const StrokeSettings&, const StrokeSettings&) = default;
};

struct Stamp {
    Vec3d position;
    StrokeFrame frame;
    double radius{};
    double opacity{};
    double hardness{};
    double rotation_radians{};
    double elongation{};
    double flow{};
    std::string tip_resource_identity;
    std::uint64_t source_ordinal{};
    std::uint64_t symmetry_instance{};
    std::uint64_t ordinal{};
    friend bool operator==(const Stamp&, const Stamp&) = default;
};

struct SweptSegment {
    std::uint64_t start_stamp_ordinal{};
    std::uint64_t end_stamp_ordinal{};
    friend constexpr bool operator==(SweptSegment, SweptSegment) noexcept = default;
};

struct ResolvedStroke {
    std::uint32_t reconstruction_version{};
    TipMode tip_mode{};
    std::uint64_t symmetry_instance_count{1};
    std::vector<Stamp> stamps;
    std::vector<SweptSegment> swept_segments;
    friend bool operator==(const ResolvedStroke&, const ResolvedStroke&) = default;
};

[[nodiscard]] ResolvedStroke ingest_resolved_stroke(const ResolvedStroke& stroke);

class StrokeResolutionError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class StrokeResolver {
public:
    explicit StrokeResolver(StrokeSettings settings = {});
    StrokeResolver(StrokeResolver&&) noexcept = default;
    StrokeResolver& operator=(StrokeResolver&&) noexcept = default;
    StrokeResolver(const StrokeResolver&) = delete;
    StrokeResolver& operator=(const StrokeResolver&) = delete;

    void append_samples(std::span<const StrokeInputSample> samples);
    [[nodiscard]] ResolvedStroke resolve();

    [[nodiscard]] const StrokeSettings& settings() const noexcept { return settings_; }
    [[nodiscard]] const ToolParameterReport& parameter_report() const noexcept {
        return parameter_report_;
    }
    [[nodiscard]] std::size_t sample_count() const noexcept { return samples_.size(); }
    [[nodiscard]] bool is_resolved() const noexcept { return resolved_; }

private:
    StrokeSettings settings_;
    ToolParameterReport parameter_report_;
    std::vector<StrokeInputSample> samples_;
    bool resolved_{};
};

}  // namespace ctex::paint

#endif
