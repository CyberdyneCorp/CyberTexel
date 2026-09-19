#ifndef CTEX_IO_TEXTURE_EXPORT_HPP
#define CTEX_IO_TEXTURE_EXPORT_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/seam_dilation.hpp>
#include <ctex/io/export_plan.hpp>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::io {

enum class ExportResampleFilter : std::uint8_t { bilinear };
enum class ExportReportState : std::uint8_t { planned, encoded, not_started };

struct ExportPixelSource {
    std::uint32_t width{};
    std::uint32_t height{};
    std::function<ExportChannelSample(std::uint32_t x, std::uint32_t y)> sample;
    std::vector<std::uint8_t> coverage;
};

using ExportPixelProvider = std::function<ExportPixelSource(const PlannedTextureExport& output)>;

struct TextureExportOptions {
    ExportPlanRequest plan;
    ExportResampleFilter resample_filter{ExportResampleFilter::bilinear};
    std::uint32_t padding_radius{image::default_uv_dilation_radius};
    int jpeg_quality{90};
    bool dry_run{};
    friend bool operator==(const TextureExportOptions&, const TextureExportOptions&) = default;
};

struct TextureExportProgress {
    std::size_t completed_outputs{};
    std::size_t total_outputs{};
    std::string relative_path;
    friend bool operator==(const TextureExportProgress&, const TextureExportProgress&) = default;
};

using TextureExportProgressCallback = std::function<void(const TextureExportProgress&)>;
using TextureExportCancellation = std::function<bool()>;

struct TextureExportReportEntry {
    std::string relative_path;
    std::vector<std::string> texture_set_identifiers;
    std::optional<std::uint32_t> udim_tile;
    std::optional<std::string> atlas_identifier;
    std::string preset_texture_suffix;
    std::uint32_t width{};
    std::uint32_t height{};
    ExportImageFormat format{ExportImageFormat::png};
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    ExportBitDepth bit_depth{ExportBitDepth::bits_8};
    std::optional<int> jpeg_quality;
    std::size_t estimated_size_bytes{};
    std::optional<std::size_t> encoded_size_bytes;
    ExportReportState state{ExportReportState::not_started};
    friend bool operator==(const TextureExportReportEntry&,
                           const TextureExportReportEntry&) = default;
};

struct TextureExportReport {
    std::string preset_identifier;
    bool dry_run{};
    bool cancelled{};
    std::vector<TextureExportReportEntry> outputs;
    friend bool operator==(const TextureExportReport&, const TextureExportReport&) = default;
};

struct InMemoryTextureExport {
    std::size_t report_entry_index{};
    std::string relative_path;
    std::uint32_t width{};
    std::uint32_t height{};
    ExportImageFormat format{ExportImageFormat::png};
    image::ColorSpace color_space{image::ColorSpace::linear_rec709};
    ExportBitDepth bit_depth{ExportBitDepth::bits_8};
    std::vector<std::byte> bytes;
    friend bool operator==(const InMemoryTextureExport&, const InMemoryTextureExport&) = default;
};

struct TextureExportResult {
    TextureExportReport report;
    std::vector<InMemoryTextureExport> buffers;
    friend bool operator==(const TextureExportResult&, const TextureExportResult&) = default;
};

enum class TextureExportErrorCode : std::uint8_t {
    invalid_option,
    invalid_source,
    over_limit,
};

class TextureExportError : public std::runtime_error {
public:
    TextureExportError(TextureExportErrorCode code, std::string message);
    [[nodiscard]] TextureExportErrorCode code() const noexcept { return code_; }

private:
    TextureExportErrorCode code_;
};

[[nodiscard]] TextureExportResult export_textures_to_memory(
    const ExportSourceCatalogue& catalogue, const ExportPreset& preset,
    const TextureExportOptions& options, const ExportPixelProvider& pixel_provider = {},
    const TextureExportProgressCallback& progress = {},
    const TextureExportCancellation& cancellation = {});

[[nodiscard]] std::string texture_export_report_json(const TextureExportReport& report);

}  // namespace ctex::io

#endif
