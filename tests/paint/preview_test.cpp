#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctex/paint/preview.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::array<std::byte, sizeof(float)> float_bytes(float value) {
    std::array<std::byte, sizeof(float)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

std::array<std::byte, sizeof(std::uint16_t)> uint16_bytes(std::uint16_t value) {
    std::array<std::byte, sizeof(std::uint16_t)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    return bytes;
}

float read_float(const image::TiledImage& pixels, std::uint32_t x) {
    float value{};
    const auto bytes = pixels.read_pixel(x, 0);
    std::memcpy(&value, bytes.data(), sizeof(value));
    return value;
}

doc::TextureChannels height_channel(std::uint32_t width) {
    doc::TextureChannels channels(width, 1, 32, doc::metallic_roughness_channels());
    channels.enable("pbr.height", 32);
    return channels;
}

bool quantized_preview_formats_round_trip_exactly() {
    doc::TextureChannels eight_bit(1, 1, 8, doc::metallic_roughness_channels());
    eight_bit.enable("pbr.height", 8);
    paint::PaintPreviewSession eight_bit_preview(eight_bit, "pbr.height");
    const std::array eight_bit_value{std::byte{173}};
    eight_bit_preview.write_pixel(0, 0, eight_bit_value);
    const std::array<std::uint8_t, 1> coverage{1};
    static_cast<void>(eight_bit_preview.finalize(coverage, 0));
    static_cast<void>(eight_bit_preview.commit(eight_bit));

    doc::TextureChannels sixteen_bit(1, 1, 16, doc::metallic_roughness_channels());
    sixteen_bit.enable("pbr.height", 16);
    paint::PaintPreviewSession sixteen_bit_preview(sixteen_bit, "pbr.height");
    const auto sixteen_bit_value = uint16_bytes(42405);
    sixteen_bit_preview.write_pixel(0, 0, sixteen_bit_value);
    static_cast<void>(sixteen_bit_preview.finalize(coverage, 0));
    static_cast<void>(sixteen_bit_preview.commit(sixteen_bit));

    const auto eight_bit_result = eight_bit.pixels("pbr.height").read_pixel(0, 0);
    const auto sixteen_bit_result = sixteen_bit.pixels("pbr.height").read_pixel(0, 0);
    return expect(
        std::equal(eight_bit_result.begin(), eight_bit_result.end(), eight_bit_value.begin()) &&
            std::equal(sixteen_bit_result.begin(), sixteen_bit_result.end(),
                       sixteen_bit_value.begin()),
        "quantized preview changed bytes between finalization and commit");
}

bool final_preview_is_isolated_and_becomes_the_exact_commit() {
    doc::TextureChannels document = height_channel(5);
    image::TiledImage& committed = document.pixels("pbr.height");
    const image::RevisionCursor baseline = committed.revision_cursor();
    paint::PaintPreviewSession session(document, "pbr.height");
    session.write_pixel(1, 0, float_bytes(0.2F));
    session.write_pixel(2, 0, float_bytes(0.3F));
    session.write_pixel(3, 0, float_bytes(0.4F));
    const image::TiledImage& provisional = session.provisional_preview();
    const bool provisional_isolated = session.state() == paint::PaintPreviewState::provisional &&
                                      std::abs(read_float(provisional, 2) - 0.3F) < 1.0e-6F &&
                                      committed.revision_cursor() == baseline &&
                                      read_float(committed, 2) == 0.0F;

    const std::array<std::uint8_t, 5> coverage{0, 1, 1, 1, 0};
    const image::TiledImage& final = session.finalize(coverage);
    std::vector<std::vector<std::byte>> final_pixels;
    for (std::uint32_t x = 0; x < final.width(); ++x) {
        const auto pixel = final.read_pixel(x, 0);
        final_pixels.emplace_back(pixel.begin(), pixel.end());
    }
    const bool final_isolated =
        session.state() == paint::PaintPreviewState::final && session.dilated_texel_count() == 2 &&
        session.zero_gradient_texel_count() == 0 &&
        std::abs(read_float(final, 0) - 0.1F) < 1.0e-6F &&
        std::abs(read_float(final, 4) - 0.5F) < 1.0e-6F &&
        committed.revision_cursor() == baseline && read_float(committed, 0) == 0.0F;

    const paint::PaintPreviewCommitReport report = session.commit(document);
    bool exact_pixels = true;
    for (std::uint32_t x = 0; x < committed.width(); ++x) {
        const auto pixel = committed.read_pixel(x, 0);
        exact_pixels &= std::equal(pixel.begin(), pixel.end(), final_pixels[x].begin());
    }
    return expect(provisional_isolated,
                  "in-flight preview modified the document or hid its provisional result") &&
           expect(final_isolated, "final preview omitted dilation or modified the document") &&
           expect(session.state() == paint::PaintPreviewState::committed && exact_pixels,
                  "commit pixels differ from the last finalized preview") &&
           expect(report.baseline == baseline && report.preview == report.committed &&
                      report.maximum_component_error == paint::paint_preview_commit_tolerance &&
                      report.changed_tiles == std::vector<image::TileCoordinate>{{0, 0}},
                  "preview commit parity or changed-tile report is incorrect");
}

bool finalization_uses_and_reports_the_bounded_dilation_radius() {
    doc::TextureChannels document = height_channel(3);
    paint::PaintPreviewSession session(document, "pbr.height");
    session.write_pixel(1, 0, float_bytes(0.5F));
    const std::array<std::uint8_t, 3> coverage{0, 1, 0};
    const std::uint32_t supplied = paint::maximum_seam_dilation_radius + 1;
    const image::TiledImage& final = session.finalize(coverage, supplied);
    return expect(session.dilation_radius() == paint::maximum_seam_dilation_radius &&
                      session.parameter_report().clamp_for("seam_dilation.radius") ==
                          paint::ToolParameterClamp{"seam_dilation.radius", supplied,
                                                    paint::maximum_seam_dilation_radius} &&
                      std::abs(read_float(final, 0) - 0.5F) < 1.0e-6F,
                  "preview finalization bypassed shared dilation-radius resolution");
}

bool document_changes_make_the_preview_stale() {
    doc::TextureChannels document = height_channel(2);
    paint::PaintPreviewSession session(document, "pbr.height");
    session.write_pixel(0, 0, float_bytes(0.25F));
    document.pixels("pbr.height").write_pixel(1, 0, float_bytes(0.75F));
    const std::array<std::uint8_t, 2> coverage{1, 1};
    static_cast<void>(session.finalize(coverage));
    bool refused = false;
    try {
        static_cast<void>(session.commit(document));
    } catch (const std::logic_error&) {
        refused = true;
    }
    return expect(refused && session.state() == paint::PaintPreviewState::final &&
                      std::abs(read_float(document.pixels("pbr.height"), 1) - 0.75F) < 1.0e-6F,
                  "stale preview overwrote a newer document channel revision");
}

bool invalid_finalization_is_transactional_and_final_is_immutable() {
    doc::TextureChannels document = height_channel(2);
    paint::PaintPreviewSession session(document, "pbr.height");
    session.write_pixel(0, 0, float_bytes(0.5F));
    bool invalid_refused = false;
    try {
        const std::array<std::uint8_t, 1> incomplete_coverage{1};
        static_cast<void>(session.finalize(incomplete_coverage));
    } catch (const std::invalid_argument&) {
        invalid_refused = true;
    }
    const bool unchanged = session.state() == paint::PaintPreviewState::provisional &&
                           std::abs(read_float(session.preview_pixels(), 0) - 0.5F) < 1.0e-6F;
    const std::array<std::uint8_t, 2> coverage{1, 1};
    static_cast<void>(session.finalize(coverage, 0));
    bool late_write_refused = false;
    try {
        session.write_pixel(1, 0, float_bytes(1.0F));
    } catch (const std::logic_error&) {
        late_write_refused = true;
    }
    return expect(invalid_refused && unchanged,
                  "invalid finalization partially changed the preview session") &&
           expect(late_write_refused && read_float(document.pixels("pbr.height"), 0) == 0.0F,
                  "final preview accepted more paint or leaked into the document");
}

bool cancellation_discards_the_in_flight_result() {
    doc::TextureChannels document = height_channel(1);
    const image::RevisionCursor baseline = document.channel_revision_cursor("pbr.height");
    paint::PaintPreviewSession session(document, "pbr.height");
    session.write_pixel(0, 0, float_bytes(1.0F));
    session.cancel();
    bool commit_refused = false;
    try {
        static_cast<void>(session.commit(document));
    } catch (const std::logic_error&) {
        commit_refused = true;
    }
    return expect(commit_refused && session.state() == paint::PaintPreviewState::cancelled &&
                      document.channel_revision_cursor("pbr.height") == baseline &&
                      read_float(document.pixels("pbr.height"), 0) == 0.0F,
                  "cancelled preview changed or could still commit to the document");
}

}  // namespace

int main() {
    return quantized_preview_formats_round_trip_exactly() &&
                   final_preview_is_isolated_and_becomes_the_exact_commit() &&
                   finalization_uses_and_reports_the_bounded_dilation_radius() &&
                   document_changes_make_the_preview_stale() &&
                   invalid_finalization_is_transactional_and_final_is_immutable() &&
                   cancellation_discards_the_in_flight_result()
               ? 0
               : 1;
}
