#ifndef CTEX_PAINT_STROKE_HPP
#define CTEX_PAINT_STROKE_HPP

#include <cstddef>
#include <cstdint>
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
        .enabled = true, .curve = {}, .minimum_output = 0.01, .maximum_output = 1.0};
    ResponseMapping pressure_opacity;
    ResponseMapping pressure_hardness;
    ResponseMapping pressure_flow;
    ResponseMapping pressure_rotation;
    ResponseMapping tilt_rotation;
    ResponseMapping tilt_elongation{
        .enabled = false, .curve = {}, .minimum_output = 1.0, .maximum_output = 2.0};
    friend bool operator==(const StrokeInputMapping&, const StrokeInputMapping&) = default;
};

struct JitterSettings {
    std::uint64_t seed{};
    double position_fraction{};
    double radius_fraction{};
    double rotation_radians{};
    double opacity{};
    double flow{};
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
    double floor{};
    bool affect_radius{true};
    bool affect_opacity{true};
    friend constexpr bool operator==(TaperSettings, TaperSettings) noexcept = default;
};

enum class ConstraintMode : std::uint8_t { none, straight_line, dominant_axis, grid };

struct ConstraintSettings {
    ConstraintMode mode{ConstraintMode::none};
    double grid_step{1.0};
    friend constexpr bool operator==(ConstraintSettings, ConstraintSettings) noexcept = default;
};

enum class SymmetryAxis : std::uint8_t { x, y, z };

struct SymmetrySettings {
    bool mirror_x{};
    bool mirror_y{};
    bool mirror_z{};
    std::uint32_t radial_count{1};
    SymmetryAxis radial_axis{SymmetryAxis::z};
    friend constexpr bool operator==(SymmetrySettings, SymmetrySettings) noexcept = default;
};

struct StrokeSettings {
    std::uint32_t reconstruction_version{canonical_stroke_reconstruction_version};
    TipMode tip_mode{TipMode::continuous_sweep};
    double spacing_fraction{default_spacing_fraction};
    double radius{1.0};
    double opacity{1.0};
    double hardness{1.0};
    double rotation_radians{};
    double elongation{1.0};
    double flow{1.0};
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
    [[nodiscard]] std::size_t sample_count() const noexcept { return samples_.size(); }
    [[nodiscard]] bool is_resolved() const noexcept { return resolved_; }

private:
    StrokeSettings settings_;
    std::vector<StrokeInputSample> samples_;
    bool resolved_{};
};

}  // namespace ctex::paint

#endif
