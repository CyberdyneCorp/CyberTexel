#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctex/io/image_io.hpp>
#include <ctex/io/texture_export.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ExportSourceCatalogue catalogue(std::size_t texture_set_count = 1) {
    ExportSourceCatalogue result{.project_name = "Export Test", .texture_sets = {}, .atlases = {}};
    for (std::size_t index = 0; index < texture_set_count; ++index) {
        result.texture_sets.push_back({.identifier = "set-" + std::to_string(index),
                                       .display_name = "Set " + std::to_string(index),
                                       .width = 4,
                                       .height = 4,
                                       .occupied_udim_tiles = {1001},
                                       .layers = {}});
    }
    return result;
}

ExportTexturePreset texture(std::string suffix, std::array<std::string_view, 4> tokens,
                            image::ColorSpace color_space = image::ColorSpace::linear_rec709) {
    return {.suffix = std::move(suffix),
            .rgba = {parse_export_channel_token(tokens[0]), parse_export_channel_token(tokens[1]),
                     parse_export_channel_token(tokens[2]), parse_export_channel_token(tokens[3])},
            .color_space = color_space,
            .bit_depth = ExportBitDepth::bits_8,
            .format = ExportImageFormat::png};
}

ExportPreset one_texture_preset(image::ColorSpace color_space = image::ColorSpace::linear_rec709) {
    return {
        .identifier = "test",
        .display_name = "Test",
        .textures = {texture("_Color", {"base_color.r", "base_color.g", "base_color.b", "opacity"},
                             color_space)}};
}

ExportPixelSource gradient_source(std::uint32_t width, std::uint32_t height,
                                  std::vector<std::uint8_t> coverage = {}) {
    return {
        .width = width,
        .height = height,
        .sample =
            [width, height](std::uint32_t x, std::uint32_t y) {
                ExportChannelSample sample;
                sample.base_color = {static_cast<double>(x) / std::max(1U, width - 1),
                                     static_cast<double>(y) / std::max(1U, height - 1), 0.0};
                sample.opacity = 1.0;
                return sample;
            },
        .coverage = std::move(coverage),
    };
}

std::uint8_t channel(const image::TiledImage& pixels, std::uint32_t x, std::uint32_t y,
                     std::size_t component) {
    return std::to_integer<std::uint8_t>(pixels.read_pixel(x, y)[component]);
}

bool export_resolution_uses_documented_bilinear_filter() {
    TextureExportOptions options;
    options.plan.output_resolution = ExportResolution{2, 2};
    options.padding_radius = 0;
    std::vector<TextureExportProgress> progress;
    const TextureExportResult result = export_textures_to_memory(
        catalogue(), one_texture_preset(), options,
        [](const PlannedTextureExport&) { return gradient_source(4, 4); },
        [&](const TextureExportProgress& value) { progress.push_back(value); });
    const auto decoded = decode_image_memory(
        {.bytes = result.buffers.front().bytes, .source_name = "downscaled.png"});
    return expect(result.buffers.size() == 1 && result.report.outputs.size() == 1,
                  "in-memory export did not return one buffer and report entry") &&
           expect(result.buffers.front().relative_path ==
                          result.report.outputs.front().relative_path &&
                      result.buffers.front().width == 2 && result.buffers.front().height == 2 &&
                      result.buffers.front().format == ExportImageFormat::png &&
                      result.buffers.front().color_space == image::ColorSpace::linear_rec709 &&
                      result.buffers.front().bit_depth == ExportBitDepth::bits_8,
                  "in-memory output omitted its declared image metadata") &&
           expect(decoded.pixels.width() == 2 && decoded.pixels.height() == 2,
                  "independent export resolution was not applied") &&
           expect(std::abs(static_cast<int>(channel(decoded.pixels, 0, 0, 0)) - 43) <= 1 &&
                      std::abs(static_cast<int>(channel(decoded.pixels, 1, 0, 0)) - 213) <= 1 &&
                      std::abs(static_cast<int>(channel(decoded.pixels, 0, 1, 1)) - 213) <= 1,
                  "export did not use pixel-centred bilinear resampling") &&
           expect(result.report.outputs.front().relative_path.find("_2_8_") != std::string::npos,
                  "resolution filename token did not use the export resolution") &&
           expect(result.report.outputs.front().encoded_size_bytes ==
                          result.buffers.front().bytes.size() &&
                      result.report.outputs.front().state == ExportReportState::encoded,
                  "machine report did not record the encoded in-memory output") &&
           expect(progress ==
                      std::vector<TextureExportProgress>{
                          {1, 1, result.report.outputs.front().relative_path}},
                  "export progress was not reported per completed output");
}

bool padding_reuses_gradient_extrapolation() {
    TextureExportOptions options;
    options.plan.output_resolution = ExportResolution{4, 1};
    options.padding_radius = 2;
    const std::vector<std::uint8_t> coverage{1, 1, 0, 0};
    const TextureExportResult result = export_textures_to_memory(
        catalogue(), one_texture_preset(), options, [&](const PlannedTextureExport&) {
            ExportPixelSource source = gradient_source(4, 1, coverage);
            source.sample = [](std::uint32_t x, std::uint32_t) {
                const double red = x < 2 ? 0.2 * (x + 1) : 0.0;
                ExportChannelSample sample;
                sample.base_color = {red, 0.0, 0.0};
                sample.opacity = 1.0;
                return sample;
            };
            return source;
        });
    const auto decoded =
        decode_image_memory({.bytes = result.buffers.front().bytes, .source_name = "padded.png"});
    return expect(std::abs(static_cast<int>(channel(decoded.pixels, 0, 0, 0)) - 51) <= 1 &&
                      std::abs(static_cast<int>(channel(decoded.pixels, 1, 0, 0)) - 102) <= 1 &&
                      std::abs(static_cast<int>(channel(decoded.pixels, 2, 0, 0)) - 153) <= 1 &&
                      std::abs(static_cast<int>(channel(decoded.pixels, 3, 0, 0)) - 204) <= 1,
                  "export padding did not use the paint engine's gradient extrapolation");
}

bool dry_run_reports_every_output_without_sampling() {
    TextureExportOptions options;
    options.dry_run = true;
    options.plan.output_resolution = ExportResolution{1024, 1024};
    bool sampled = false;
    const TextureExportResult result = export_textures_to_memory(
        catalogue(2), default_export_preset(), options, [&](const PlannedTextureExport&) {
            sampled = true;
            return gradient_source(1, 1);
        });
    const std::string json = texture_export_report_json(result.report);
    return expect(!sampled && result.buffers.empty(),
                  "dry run sampled pixels or returned encoded buffers") &&
           expect(result.report.outputs.size() == default_export_preset().textures.size() * 2,
                  "dry run did not report the complete output manifest") &&
           expect(std::all_of(result.report.outputs.begin(), result.report.outputs.end(),
                              [](const auto& output) {
                                  return output.width == 1024 && output.height == 1024 &&
                                         output.estimated_size_bytes > 0 &&
                                         output.state == ExportReportState::planned &&
                                         !output.encoded_size_bytes.has_value();
                              }),
                  "dry-run report omitted dimensions, estimates, or planned state") &&
           expect(json.find("\"preset\":\"pbr-individual\"") != std::string::npos &&
                      json.find("\"format\":\"PNG\"") != std::string::npos &&
                      json.find("\"color_space\":") != std::string::npos &&
                      json.find("\"bit_depth\":") != std::string::npos,
                  "machine-readable report omitted required fields");
}

bool cancellation_keeps_only_complete_buffers() {
    ExportPreset preset = one_texture_preset();
    preset.textures.push_back(texture("_Second", {"roughness", "metallic", "occlusion", "1.0"}));
    TextureExportOptions options;
    options.plan.output_resolution = ExportResolution{1, 1};
    options.padding_radius = 0;
    std::size_t completed = 0;
    const TextureExportResult result = export_textures_to_memory(
        catalogue(), preset, options,
        [](const PlannedTextureExport&) { return gradient_source(1, 1); },
        [&](const TextureExportProgress&) { ++completed; }, [&] { return completed == 1; });
    return expect(result.report.cancelled && result.buffers.size() == 1 && completed == 1,
                  "cancelled export retained a partial output or ignored cancellation") &&
           expect(result.report.outputs[0].state == ExportReportState::encoded &&
                      result.report.outputs[1].state == ExportReportState::not_started,
                  "cancelled report did not distinguish complete and unstarted outputs");
}

bool registered_data_channels_skip_color_transfer() {
    const ExportPreset preset{
        .identifier = "registered",
        .display_name = "Registered",
        .textures = {texture("_Registered",
                             {"channel:pbr.coat_weight:0", "base_color.r", "0.0", "1.0"},
                             image::ColorSpace::srgb_rec709)},
    };
    TextureExportOptions options;
    options.plan.output_resolution = ExportResolution{1, 1};
    options.padding_radius = 0;
    const TextureExportResult result =
        export_textures_to_memory(catalogue(), preset, options, [](const PlannedTextureExport&) {
            ExportPixelSource source = gradient_source(1, 1);
            source.sample = [](std::uint32_t, std::uint32_t) {
                ExportChannelSample sample;
                sample.base_color = {0.25, 0.0, 0.0};
                sample.registered_channels = {
                    {"pbr.coat_weight",
                     {.component_count = 1, .components = {0.25, 0.0, 0.0, 0.0}}}};
                return sample;
            };
            return source;
        });
    const auto decoded = decode_image_memory(
        {.bytes = result.buffers.front().bytes, .source_name = "registered.png"});
    return expect(std::abs(static_cast<int>(channel(decoded.pixels, 0, 0, 0)) - 64) <= 1,
                  "registered data channel received an sRGB transfer function") &&
           expect(std::abs(static_cast<int>(channel(decoded.pixels, 0, 0, 1)) - 137) <= 1,
                  "base-colour token did not receive the requested sRGB transfer function");
}

bool jpeg_quality_is_reported() {
    ExportPreset preset = one_texture_preset();
    preset.textures.front().format = ExportImageFormat::jpeg;
    TextureExportOptions options;
    options.dry_run = true;
    options.jpeg_quality = 73;
    const TextureExportResult result = export_textures_to_memory(catalogue(), preset, options);
    const std::string json = texture_export_report_json(result.report);
    return expect(result.report.outputs.front().jpeg_quality == 73,
                  "JPEG quality was not retained in the export report") &&
           expect(json.find("\"jpeg_quality\":73") != std::string::npos,
                  "machine-readable report omitted JPEG quality");
}

bool write_report(std::string_view path) {
    TextureExportOptions options;
    options.dry_run = true;
    const TextureExportResult result =
        export_textures_to_memory(catalogue(), one_texture_preset(), options);
    std::ofstream output(std::string(path), std::ios::binary);
    output << texture_export_report_json(result.report);
    return expect(static_cast<bool>(output), "failed to write texture export report fixture");
}

bool write_determinism_outputs() {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return expect(false, "CTEX_DETERMINISM_OUTPUT_DIR is required");
    }
    TextureExportOptions options;
    options.padding_radius = 0;
    const TextureExportResult result = export_textures_to_memory(
        catalogue(), one_texture_preset(), options,
        [](const PlannedTextureExport&) { return gradient_source(4, 4); });
    const std::filesystem::path root(directory);
    std::ofstream report(root / "texture-export-report.json", std::ios::binary);
    report << texture_export_report_json(result.report);
    std::ofstream image(root / "texture-export.png", std::ios::binary);
    const std::vector<std::byte>& bytes = result.buffers.front().bytes;
    image.write(reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
    return expect(static_cast<bool>(report) && static_cast<bool>(image),
                  "failed to write texture export determinism artifacts");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "--write-report") {
        return write_report(argv[2]) ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--write-determinism") {
        return write_determinism_outputs() ? 0 : 1;
    }
    return export_resolution_uses_documented_bilinear_filter() &&
                   padding_reuses_gradient_extrapolation() &&
                   dry_run_reports_every_output_without_sampling() &&
                   cancellation_keeps_only_complete_buffers() &&
                   registered_data_channels_skip_color_transfer() && jpeg_quality_is_reported()
               ? 0
               : 1;
}
