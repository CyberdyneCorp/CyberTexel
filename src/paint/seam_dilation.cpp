#include <algorithm>
#include <cmath>
#include <ctex/paint/seam_dilation.hpp>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctex::paint {
namespace {

std::size_t checked_texel_count(const SeamDilationRaster& raster) {
    if (raster.width == 0 || raster.height == 0 || raster.component_count == 0 ||
        raster.component_count > 4 ||
        static_cast<std::size_t>(raster.width) >
            std::numeric_limits<std::size_t>::max() / raster.height) {
        throw std::invalid_argument("seam-dilation raster dimensions or components are invalid");
    }
    const std::size_t texel_count = static_cast<std::size_t>(raster.width) * raster.height;
    if (texel_count > std::numeric_limits<std::size_t>::max() / raster.component_count ||
        raster.pixels.size() != texel_count * raster.component_count ||
        !std::all_of(raster.pixels.begin(), raster.pixels.end(),
                     [](double value) { return std::isfinite(value); })) {
        throw std::invalid_argument("seam-dilation raster pixels are invalid");
    }
    return texel_count;
}

void validate_coverage(std::span<const std::uint8_t> coverage, std::size_t texel_count) {
    if (coverage.size() != texel_count ||
        !std::all_of(coverage.begin(), coverage.end(),
                     [](std::uint8_t value) { return value == 0 || value == 1; })) {
        throw std::invalid_argument("seam-dilation coverage must be one binary value per texel");
    }
}

DilatedUvTile provisional_tile(UvTileCoordinate coordinate, const SeamDilationRaster& raster) {
    return {.coordinate = coordinate,
            .raster = raster,
            .dilated_texel_count = 0,
            .zero_gradient_texel_count = 0};
}

}  // namespace

std::uint32_t resolve_seam_dilation_radius(std::uint32_t requested, ToolParameterReport& report) {
    return static_cast<std::uint32_t>(
        validate_tool_parameter(seam_dilation_radius_parameter, requested, report));
}

SeamDilationResult dilate_uv_seams(const SeamDilationRaster& source,
                                   std::span<const std::uint8_t> coverage, std::uint32_t radius) {
    ToolParameterReport parameter_report;
    radius = resolve_seam_dilation_radius(radius, parameter_report);
    image::SeamDilationPixels pixels = image::extrapolate_uv_seams(source, coverage, radius);
    return {.raster = std::move(pixels.raster),
            .dilated_texel_count = pixels.dilated_texel_count,
            .zero_gradient_texel_count = pixels.zero_gradient_texel_count,
            .radius = radius,
            .parameter_report = std::move(parameter_report)};
}

class DeferredStrokeDilation::Impl {
public:
    explicit Impl(std::uint32_t radius)
        : radius_(resolve_seam_dilation_radius(radius, parameter_report_)) {}

    void stage_tile(UvTileCoordinate coordinate, const SeamDilationRaster& raster,
                    std::span<const std::uint8_t> coverage) {
        if (finished_) {
            throw std::logic_error("cannot stage a UV tile after stroke dilation finished");
        }
        validate_coverage(coverage, checked_texel_count(raster));
        StagedTile staged{.coordinate = coordinate,
                          .raster = raster,
                          .coverage = {coverage.begin(), coverage.end()}};
        const auto existing = std::lower_bound(
            staged_.begin(), staged_.end(), coordinate,
            [](const StagedTile& tile, UvTileCoordinate value) { return tile.coordinate < value; });
        if (existing == staged_.end()) {
            staged_.push_back(std::move(staged));
        } else if (existing->coordinate != coordinate) {
            staged_.insert(existing, std::move(staged));
        } else {
            *existing = std::move(staged);
        }
    }

    StrokeDilationOutput provisional_preview() const {
        if (finished_) {
            return output_;
        }
        StrokeDilationOutput preview{.state = DilationPreviewState::provisional,
                                     .tiles = {},
                                     .dilation_pass_count = 0,
                                     .radius = radius_,
                                     .parameter_report = parameter_report_};
        preview.tiles.reserve(staged_.size());
        for (const StagedTile& tile : staged_) {
            preview.tiles.push_back(provisional_tile(tile.coordinate, tile.raster));
        }
        return preview;
    }

    const StrokeDilationOutput& finish() {
        if (finished_) {
            return output_;
        }
        StrokeDilationOutput completed{.state = DilationPreviewState::final,
                                       .tiles = {},
                                       .dilation_pass_count = 0,
                                       .radius = radius_,
                                       .parameter_report = parameter_report_};
        completed.tiles.reserve(staged_.size());
        for (const StagedTile& tile : staged_) {
            SeamDilationResult result = dilate_uv_seams(tile.raster, tile.coverage, radius_);
            completed.tiles.push_back(
                {.coordinate = tile.coordinate,
                 .raster = std::move(result.raster),
                 .dilated_texel_count = result.dilated_texel_count,
                 .zero_gradient_texel_count = result.zero_gradient_texel_count});
            completed.dilation_pass_count += radius_ == 0 ? 0 : 1;
        }
        output_ = std::move(completed);
        finished_ = true;
        return output_;
    }

    bool finished() const noexcept { return finished_; }
    std::uint32_t radius() const noexcept { return radius_; }
    const ToolParameterReport& parameter_report() const noexcept { return parameter_report_; }

private:
    struct StagedTile {
        UvTileCoordinate coordinate;
        SeamDilationRaster raster;
        std::vector<std::uint8_t> coverage;
    };

    ToolParameterReport parameter_report_;
    std::uint32_t radius_;
    bool finished_{};
    std::vector<StagedTile> staged_;
    StrokeDilationOutput output_;
};

DeferredStrokeDilation::DeferredStrokeDilation(std::uint32_t radius)
    : impl_(std::make_unique<Impl>(radius)) {}
DeferredStrokeDilation::~DeferredStrokeDilation() = default;
DeferredStrokeDilation::DeferredStrokeDilation(DeferredStrokeDilation&&) noexcept = default;
DeferredStrokeDilation& DeferredStrokeDilation::operator=(DeferredStrokeDilation&&) noexcept =
    default;

void DeferredStrokeDilation::stage_tile(UvTileCoordinate coordinate,
                                        const SeamDilationRaster& raster,
                                        std::span<const std::uint8_t> coverage) {
    impl_->stage_tile(coordinate, raster, coverage);
}

StrokeDilationOutput DeferredStrokeDilation::provisional_preview() const {
    return impl_->provisional_preview();
}

const StrokeDilationOutput& DeferredStrokeDilation::finish() { return impl_->finish(); }

bool DeferredStrokeDilation::finished() const noexcept { return impl_->finished(); }

std::uint32_t DeferredStrokeDilation::radius() const noexcept { return impl_->radius(); }

const ToolParameterReport& DeferredStrokeDilation::parameter_report() const noexcept {
    return impl_->parameter_report();
}

}  // namespace ctex::paint
