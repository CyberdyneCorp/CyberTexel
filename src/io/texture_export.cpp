#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/image/color.hpp>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/texture_encode.hpp>
#include <ctex/io/texture_export.hpp>
#include <limits>
#include <string_view>
#include <utility>

namespace ctex::io {
namespace {

struct ExportCancelled {};

void cancellation_point(const TextureExportCancellation& cancellation) {
    if (cancellation && cancellation()) {
        throw ExportCancelled{};
    }
}

std::size_t checked_multiply(std::size_t left, std::size_t right, std::string_view subject) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw TextureExportError(TextureExportErrorCode::over_limit,
                                 std::string(subject) + " size overflows");
    }
    return left * right;
}

std::size_t checked_add(std::size_t left, std::size_t right, std::string_view subject) {
    if (left > std::numeric_limits<std::size_t>::max() - right) {
        throw TextureExportError(TextureExportErrorCode::over_limit,
                                 std::string(subject) + " size overflows");
    }
    return left + right;
}

std::size_t texel_count(std::uint32_t width, std::uint32_t height, std::string_view subject) {
    if (width == 0 || height == 0) {
        throw TextureExportError(TextureExportErrorCode::invalid_source,
                                 std::string(subject) + " dimensions must be non-zero");
    }
    return checked_multiply(width, height, subject);
}

std::size_t estimate_encoded_size(const PlannedTextureExport& output) {
    const std::size_t raw = checked_multiply(
        checked_multiply(texel_count(output.width, output.height, "export estimate"), 4,
                         "export estimate"),
        static_cast<unsigned>(output.bit_depth) / 8, "export estimate");
    std::size_t overhead = 512;
    switch (output.format) {
        case ExportImageFormat::png:
        case ExportImageFormat::jpeg:
            overhead = checked_add(65'536, raw / 16, "export estimate");
            break;
        case ExportImageFormat::tga:
            overhead = checked_add(64, raw / 64, "export estimate");
            break;
        case ExportImageFormat::tiff:
            break;
        case ExportImageFormat::openexr:
            overhead = checked_add(1'024, checked_multiply(output.height, 16, "export estimate"),
                                   "export estimate");
            break;
    }
    return checked_add(raw, overhead, "export estimate");
}

TextureExportReport make_report(const std::vector<PlannedTextureExport>& plan,
                                const ExportPreset& preset, bool dry_run, int jpeg_quality) {
    TextureExportReport report{.preset_identifier = preset.identifier,
                               .dry_run = dry_run,
                               .cancelled = false,
                               .outputs = {}};
    report.outputs.reserve(plan.size());
    for (const PlannedTextureExport& output : plan) {
        report.outputs.push_back({
            .relative_path = output.relative_path,
            .texture_set_identifiers = output.texture_set_identifiers,
            .udim_tile = output.udim_tile,
            .atlas_identifier = output.atlas_identifier,
            .preset_texture_suffix = preset.textures[output.preset_texture_index].suffix,
            .width = output.width,
            .height = output.height,
            .format = output.format,
            .color_space = output.color_space,
            .bit_depth = output.bit_depth,
            .jpeg_quality = output.format == ExportImageFormat::jpeg
                                ? std::optional<int>{jpeg_quality}
                                : std::nullopt,
            .estimated_size_bytes = estimate_encoded_size(output),
            .encoded_size_bytes = std::nullopt,
            .state = dry_run ? ExportReportState::planned : ExportReportState::not_started,
        });
    }
    return report;
}

void validate_options(const ExportPreset& preset, const TextureExportOptions& options,
                      const ExportPixelProvider& provider) {
    if (options.resample_filter != ExportResampleFilter::bilinear) {
        throw TextureExportError(TextureExportErrorCode::invalid_option,
                                 "texture export resampling filter is invalid");
    }
    if (options.padding_radius > image::maximum_uv_dilation_radius) {
        throw TextureExportError(TextureExportErrorCode::invalid_option,
                                 "texture export padding exceeds the supported maximum");
    }
    const bool has_jpeg =
        std::any_of(preset.textures.begin(), preset.textures.end(),
                    [](const auto& texture) { return texture.format == ExportImageFormat::jpeg; });
    if (has_jpeg && (options.jpeg_quality < 1 || options.jpeg_quality > 100)) {
        throw TextureExportError(TextureExportErrorCode::invalid_option,
                                 "texture export JPEG quality must be from 1 through 100");
    }
    if (!options.dry_run && !provider) {
        throw TextureExportError(TextureExportErrorCode::invalid_option,
                                 "in-memory texture export requires a pixel provider");
    }
}

bool color_token(ExportChannelTokenKind kind) {
    switch (kind) {
        case ExportChannelTokenKind::base_color_red:
        case ExportChannelTokenKind::base_color_green:
        case ExportChannelTokenKind::base_color_blue:
        case ExportChannelTokenKind::emission_red:
        case ExportChannelTokenKind::emission_green:
        case ExportChannelTokenKind::emission_blue:
        case ExportChannelTokenKind::diffuse_red:
        case ExportChannelTokenKind::diffuse_green:
        case ExportChannelTokenKind::diffuse_blue:
        case ExportChannelTokenKind::specular_red:
        case ExportChannelTokenKind::specular_green:
        case ExportChannelTokenKind::specular_blue:
            return true;
        default:
            return false;
    }
}

struct WorkingRaster {
    image::SeamDilationRaster pixels;
    std::vector<std::uint8_t> coverage;
};

std::vector<std::uint8_t> source_coverage(const ExportPixelSource& source,
                                          std::size_t source_texels) {
    if (source.coverage.empty()) {
        return std::vector<std::uint8_t>(source_texels, 1);
    }
    if (source.coverage.size() != source_texels ||
        !std::all_of(source.coverage.begin(), source.coverage.end(),
                     [](std::uint8_t value) { return value == 0 || value == 1; })) {
        throw TextureExportError(TextureExportErrorCode::invalid_source,
                                 "texture export coverage must contain one binary value per texel");
    }
    return source.coverage;
}

WorkingRaster pack_source(const ExportPixelSource& source, const ExportTexturePreset& texture,
                          const TextureExportCancellation& cancellation) {
    const std::size_t source_texels = texel_count(source.width, source.height, "export source");
    if (!source.sample) {
        throw TextureExportError(TextureExportErrorCode::invalid_source,
                                 "texture export source has no pixel sampler");
    }
    WorkingRaster result{
        .pixels = {.width = source.width,
                   .height = source.height,
                   .component_count = 4,
                   .pixels =
                       std::vector<double>(checked_multiply(source_texels, 4, "export source"))},
        .coverage = source_coverage(source, source_texels),
    };
    for (std::uint32_t y = 0; y < source.height; ++y) {
        cancellation_point(cancellation);
        for (std::uint32_t x = 0; x < source.width; ++x) {
            const ExportChannelSample sample = source.sample(x, y);
            const std::size_t offset =
                (static_cast<std::size_t>(y) * source.width + x) * texture.rgba.size();
            for (std::size_t component = 0; component < texture.rgba.size(); ++component) {
                const double value = evaluate_export_channel_token(texture.rgba[component], sample);
                if (!std::isfinite(value)) {
                    throw TextureExportError(TextureExportErrorCode::invalid_source,
                                             "texture export source produced a non-finite value");
                }
                result.pixels.pixels[offset + component] = value;
            }
        }
    }
    return result;
}

std::uint32_t nearest_coordinate(double coordinate, std::uint32_t extent) {
    const double rounded = std::floor(coordinate + 0.5);
    return static_cast<std::uint32_t>(std::clamp(rounded, 0.0, static_cast<double>(extent - 1)));
}

struct BilinearAxis {
    std::uint32_t first{};
    std::uint32_t second{};
    double weight{};
};

BilinearAxis bilinear_axis(std::uint32_t destination, std::uint32_t source_extent,
                           std::uint32_t destination_extent) {
    const double coordinate =
        ((static_cast<double>(destination) + 0.5) * source_extent / destination_extent) - 0.5;
    const double base = std::floor(coordinate);
    return {
        .first = static_cast<std::uint32_t>(
            std::clamp(base, 0.0, static_cast<double>(source_extent - 1))),
        .second = static_cast<std::uint32_t>(
            std::clamp(base + 1.0, 0.0, static_cast<double>(source_extent - 1))),
        .weight = coordinate - base,
    };
}

double raster_component(const image::SeamDilationRaster& raster, std::uint32_t x, std::uint32_t y,
                        std::size_t component) {
    return raster.pixels[(static_cast<std::size_t>(y) * raster.width + x) * raster.component_count +
                         component];
}

WorkingRaster resample_bilinear(const WorkingRaster& source, std::uint32_t width,
                                std::uint32_t height,
                                const TextureExportCancellation& cancellation) {
    if (source.pixels.width == width && source.pixels.height == height) {
        return source;
    }
    const std::size_t output_texels = texel_count(width, height, "resampled export");
    WorkingRaster result{
        .pixels = {.width = width,
                   .height = height,
                   .component_count = source.pixels.component_count,
                   .pixels = std::vector<double>(checked_multiply(
                       output_texels, source.pixels.component_count, "resampled export"))},
        .coverage = std::vector<std::uint8_t>(output_texels),
    };
    for (std::uint32_t y = 0; y < height; ++y) {
        cancellation_point(cancellation);
        const BilinearAxis vertical = bilinear_axis(y, source.pixels.height, height);
        const double source_y =
            ((static_cast<double>(y) + 0.5) * source.pixels.height / height) - 0.5;
        for (std::uint32_t x = 0; x < width; ++x) {
            const BilinearAxis horizontal = bilinear_axis(x, source.pixels.width, width);
            const std::size_t output = static_cast<std::size_t>(y) * width + x;
            for (std::size_t component = 0; component < source.pixels.component_count;
                 ++component) {
                const double top = std::lerp(
                    raster_component(source.pixels, horizontal.first, vertical.first, component),
                    raster_component(source.pixels, horizontal.second, vertical.first, component),
                    horizontal.weight);
                const double bottom = std::lerp(
                    raster_component(source.pixels, horizontal.first, vertical.second, component),
                    raster_component(source.pixels, horizontal.second, vertical.second, component),
                    horizontal.weight);
                result.pixels.pixels[output * source.pixels.component_count + component] =
                    std::lerp(top, bottom, vertical.weight);
            }
            const double source_x =
                ((static_cast<double>(x) + 0.5) * source.pixels.width / width) - 0.5;
            const std::uint32_t coverage_x = nearest_coordinate(source_x, source.pixels.width);
            const std::uint32_t coverage_y = nearest_coordinate(source_y, source.pixels.height);
            result.coverage[output] =
                source.coverage[static_cast<std::size_t>(coverage_y) * source.pixels.width +
                                coverage_x];
        }
    }
    return result;
}

void apply_output_color_space(image::SeamDilationRaster& raster,
                              const ExportTexturePreset& texture) {
    if (texture.color_space != image::ColorSpace::srgb_rec709) {
        return;
    }
    for (std::size_t texel = 0; texel < raster.pixels.size() / raster.component_count; ++texel) {
        for (std::size_t component = 0; component < texture.rgba.size(); ++component) {
            if (color_token(texture.rgba[component].kind)) {
                raster.pixels[texel * raster.component_count + component] = image::linear_to_srgb(
                    raster.pixels[texel * raster.component_count + component]);
            }
        }
    }
}

image::TiledImage tiled_float_image(const image::SeamDilationRaster& raster,
                                    const TextureExportCancellation& cancellation) {
    image::TiledImage result(raster.width, raster.height, {image::ChannelType::float32, 4});
    for (std::uint32_t y = 0; y < raster.height; ++y) {
        cancellation_point(cancellation);
        for (std::uint32_t x = 0; x < raster.width; ++x) {
            std::array<float, 4> values{};
            const std::size_t offset = (static_cast<std::size_t>(y) * raster.width + x) * 4;
            const auto first = raster.pixels.begin() + static_cast<std::ptrdiff_t>(offset);
            std::transform(first, first + 4, values.begin(),
                           [](double value) { return static_cast<float>(value); });
            std::array<std::byte, sizeof(values)> bytes{};
            std::memcpy(bytes.data(), values.data(), bytes.size());
            result.write_pixel(x, y, bytes);
        }
    }
    return result;
}

std::vector<std::byte> encode_output(const PlannedTextureExport& output,
                                     const ExportTexturePreset& texture,
                                     const TextureExportOptions& options,
                                     const ExportPixelProvider& provider,
                                     const TextureExportCancellation& cancellation) {
    cancellation_point(cancellation);
    WorkingRaster working = pack_source(provider(output), texture, cancellation);
    working = resample_bilinear(working, output.width, output.height, cancellation);
    image::SeamDilationPixels padded =
        image::extrapolate_uv_seams(working.pixels, working.coverage, options.padding_radius);
    apply_output_color_space(padded.raster, texture);
    image::TiledImage pixels = tiled_float_image(padded.raster, cancellation);
    cancellation_point(cancellation);
    return encode_texture_memory(pixels, {.format = output.format,
                                          .bit_depth = output.bit_depth,
                                          .color_space = output.color_space,
                                          .jpeg_quality = options.jpeg_quality});
}

void append_json_string(std::string& output, std::string_view value) {
    constexpr char hex[] = "0123456789abcdef";
    output.push_back('"');
    for (const unsigned char character : value) {
        switch (character) {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (character < 0x20) {
                    output += "\\u00";
                    output.push_back(hex[character >> 4U]);
                    output.push_back(hex[character & 0x0fU]);
                } else {
                    output.push_back(static_cast<char>(character));
                }
        }
    }
    output.push_back('"');
}

std::string_view report_state_name(ExportReportState state) {
    switch (state) {
        case ExportReportState::planned:
            return "planned";
        case ExportReportState::encoded:
            return "encoded";
        case ExportReportState::not_started:
            return "not_started";
    }
    return "invalid";
}

void append_texture_sets_json(std::string& json, const std::vector<std::string>& identifiers) {
    json.push_back('[');
    for (std::size_t index = 0; index < identifiers.size(); ++index) {
        if (index != 0) {
            json.push_back(',');
        }
        append_json_string(json, identifiers[index]);
    }
    json.push_back(']');
}

void append_optional_size_json(std::string& json, std::optional<std::size_t> value) {
    if (value.has_value()) {
        json += std::to_string(*value);
    } else {
        json += "null";
    }
}

void append_optional_uint_json(std::string& json, std::optional<std::uint32_t> value) {
    if (value.has_value()) {
        json += std::to_string(*value);
    } else {
        json += "null";
    }
}

void append_optional_int_json(std::string& json, std::optional<int> value) {
    if (value.has_value()) {
        json += std::to_string(*value);
    } else {
        json += "null";
    }
}

void append_optional_string_json(std::string& json, const std::optional<std::string>& value) {
    if (value.has_value()) {
        append_json_string(json, *value);
    } else {
        json += "null";
    }
}

void append_report_entry_json(std::string& json, const TextureExportReportEntry& entry) {
    json += "{\"path\":";
    append_json_string(json, entry.relative_path);
    json += ",\"texture_sets\":";
    append_texture_sets_json(json, entry.texture_set_identifiers);
    json += ",\"udim\":";
    append_optional_uint_json(json, entry.udim_tile);
    json += ",\"atlas\":";
    append_optional_string_json(json, entry.atlas_identifier);
    json += ",\"preset_entry\":";
    append_json_string(json, entry.preset_texture_suffix);
    json += ",\"width\":" + std::to_string(entry.width);
    json += ",\"height\":" + std::to_string(entry.height);
    json += ",\"format\":";
    append_json_string(json, export_image_format_name(entry.format));
    json += ",\"color_space\":";
    append_json_string(json, image::color_space_name(entry.color_space));
    json += ",\"bit_depth\":" + std::to_string(static_cast<unsigned>(entry.bit_depth));
    json += ",\"jpeg_quality\":";
    append_optional_int_json(json, entry.jpeg_quality);
    json += ",\"estimated_size_bytes\":" + std::to_string(entry.estimated_size_bytes);
    json += ",\"encoded_size_bytes\":";
    append_optional_size_json(json, entry.encoded_size_bytes);
    json += ",\"state\":";
    append_json_string(json, report_state_name(entry.state));
    json.push_back('}');
}

}  // namespace

TextureExportError::TextureExportError(TextureExportErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

TextureExportResult export_textures_to_memory(const ExportSourceCatalogue& catalogue,
                                              const ExportPreset& preset,
                                              const TextureExportOptions& options,
                                              const ExportPixelProvider& pixel_provider,
                                              const TextureExportProgressCallback& progress,
                                              const TextureExportCancellation& cancellation) {
    validate_options(preset, options, pixel_provider);
    const std::vector<PlannedTextureExport> plan =
        plan_texture_export(catalogue, preset, options.plan);
    TextureExportResult result{
        .report = make_report(plan, preset, options.dry_run, options.jpeg_quality), .buffers = {}};
    if (options.dry_run) {
        return result;
    }
    result.buffers.reserve(plan.size());
    try {
        for (std::size_t index = 0; index < plan.size(); ++index) {
            cancellation_point(cancellation);
            std::vector<std::byte> bytes =
                encode_output(plan[index], preset.textures[plan[index].preset_texture_index],
                              options, pixel_provider, cancellation);
            cancellation_point(cancellation);
            result.report.outputs[index].encoded_size_bytes = bytes.size();
            result.report.outputs[index].state = ExportReportState::encoded;
            result.buffers.push_back({.report_entry_index = index,
                                      .relative_path = plan[index].relative_path,
                                      .width = plan[index].width,
                                      .height = plan[index].height,
                                      .format = plan[index].format,
                                      .color_space = plan[index].color_space,
                                      .bit_depth = plan[index].bit_depth,
                                      .bytes = std::move(bytes)});
            if (progress) {
                progress({.completed_outputs = result.buffers.size(),
                          .total_outputs = plan.size(),
                          .relative_path = plan[index].relative_path});
            }
        }
    } catch (const ExportCancelled&) {
        result.report.cancelled = true;
    }
    return result;
}

std::string texture_export_report_json(const TextureExportReport& report) {
    std::string json = "{\"preset\":";
    append_json_string(json, report.preset_identifier);
    json += ",\"dry_run\":";
    json += report.dry_run ? "true" : "false";
    json += ",\"cancelled\":";
    json += report.cancelled ? "true" : "false";
    json += ",\"outputs\":[";
    for (std::size_t index = 0; index < report.outputs.size(); ++index) {
        if (index != 0) {
            json.push_back(',');
        }
        append_report_entry_json(json, report.outputs[index]);
    }
    json += "]}";
    return json;
}

}  // namespace ctex::io
