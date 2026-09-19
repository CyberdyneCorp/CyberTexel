#ifndef CTEX_PAINT_SEAM_DILATION_HPP
#define CTEX_PAINT_SEAM_DILATION_HPP

#include <compare>
#include <cstddef>
#include <cstdint>
#include <ctex/paint/parameters.hpp>
#include <memory>
#include <span>
#include <vector>

namespace ctex::paint {

inline constexpr std::uint32_t maximum_seam_dilation_radius = 4'096;
inline constexpr ToolParameterDescriptor seam_dilation_radius_parameter{
    "seam_dilation.radius", 2.0, 0.0, maximum_seam_dilation_radius};
inline constexpr std::uint32_t default_seam_dilation_radius =
    static_cast<std::uint32_t>(seam_dilation_radius_parameter.default_value);

[[nodiscard]] std::uint32_t resolve_seam_dilation_radius(std::uint32_t requested,
                                                         ToolParameterReport& report);

struct SeamDilationRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint8_t component_count{};
    std::vector<double> pixels;
};

struct SeamDilationResult {
    SeamDilationRaster raster;
    std::size_t dilated_texel_count{};
    std::size_t zero_gradient_texel_count{};
    std::uint32_t radius{};
    ToolParameterReport parameter_report;
};

[[nodiscard]] SeamDilationResult dilate_uv_seams(
    const SeamDilationRaster& source, std::span<const std::uint8_t> coverage,
    std::uint32_t radius = default_seam_dilation_radius);

struct UvTileCoordinate {
    std::int32_t u{};
    std::int32_t v{};
    friend constexpr auto operator<=>(UvTileCoordinate, UvTileCoordinate) noexcept = default;
};

enum class DilationPreviewState : std::uint8_t { provisional, final };

struct DilatedUvTile {
    UvTileCoordinate coordinate;
    SeamDilationRaster raster;
    std::size_t dilated_texel_count{};
    std::size_t zero_gradient_texel_count{};
};

struct StrokeDilationOutput {
    DilationPreviewState state{DilationPreviewState::provisional};
    std::vector<DilatedUvTile> tiles;
    std::size_t dilation_pass_count{};
    std::uint32_t radius{};
    ToolParameterReport parameter_report;
};

class DeferredStrokeDilation {
public:
    explicit DeferredStrokeDilation(std::uint32_t radius = default_seam_dilation_radius);
    ~DeferredStrokeDilation();
    DeferredStrokeDilation(DeferredStrokeDilation&&) noexcept;
    DeferredStrokeDilation& operator=(DeferredStrokeDilation&&) noexcept;
    DeferredStrokeDilation(const DeferredStrokeDilation&) = delete;
    DeferredStrokeDilation& operator=(const DeferredStrokeDilation&) = delete;

    void stage_tile(UvTileCoordinate coordinate, const SeamDilationRaster& raster,
                    std::span<const std::uint8_t> coverage);
    [[nodiscard]] StrokeDilationOutput provisional_preview() const;
    [[nodiscard]] const StrokeDilationOutput& finish();
    [[nodiscard]] bool finished() const noexcept;
    [[nodiscard]] std::uint32_t radius() const noexcept;
    [[nodiscard]] const ToolParameterReport& parameter_report() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::paint

#endif
