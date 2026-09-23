#include <array>
#include <cstddef>
#include <ctex/doc/resolution_change.hpp>
#include <ctex/io/operation_record.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ResolutionChangeErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const ResolutionChangeError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

TextureSet texture_set(bool udim = false) {
    TextureSet result({.display_name = "Body",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "body",
                       .uv_set = "uv0",
                       .width = 2,
                       .height = 2,
                       .default_bit_depth = 8,
                       .udim_tiling = udim});
    result.channels().enable("pbr.base_color");
    result.channels()
        .pixels("pbr.base_color")
        .write_pixel(0, 0, std::array{std::byte{0}, std::byte{0}, std::byte{255}});
    return result;
}

ctex::io::EditableOperationRecord operation(std::string identifier,
                                            ctex::io::OperationReplayClass replay_class) {
    return {.identifier = std::move(identifier),
            .algorithm_identifier = "cybertexel.paint.brush",
            .algorithm_version = 3,
            .preset_identifier = "brushes/basic",
            .preset_version = 1,
            .replay_class = replay_class,
            .input_document_revision = 7,
            .seed = 42,
            .mesh_content_identity = "sha256:mesh",
            .payload_kind = ctex::io::OperationPayloadKind::resolved_stamps,
            .payload_version = 1,
            .channels = {{.semantic_id = "pbr.base_color",
                          .format = {ctex::image::ChannelType::uint8_unorm, 3},
                          .default_value = {0.5, 0.5, 0.5}}},
            .checkpoint_image_identifiers = {"checkpoint/" + identifier},
            .payload = {std::byte{1}}};
}

std::vector<std::byte> replayed_tip() {
    std::vector<std::byte> pixels(4 * 4 * 3, std::byte{0});
    pixels[3 * (4 * 4 - 1)] = std::byte{255};
    return pixels;
}

bool mixed_replay_rasterizes_at_target_resolution() {
    TextureSet set = texture_set();
    LayerEntry fill{
        .identifier = "fill", .display_name = "Fill", .kind = LayerEntryKind::fill_layer};
    fill.graph = ctex::graph::GraphDocument({.role = ctex::graph::NodeRole::output,
                                             .type_id = "ctex.output.resize-test",
                                             .type_version = 1,
                                             .display_name = "Output"});
    set.layer_stack().append(std::move(fill));

    const auto eligible =
        operation("stroke", ctex::io::OperationReplayClass::resolution_independent);
    const auto checkpoint = operation("smear", ctex::io::OperationReplayClass::checkpoint_only);
    const std::array support{ctex::io::OperationAlgorithmSupport{
        .identifier = "cybertexel.paint.brush", .minimum_version = 3, .maximum_version = 3}};
    const std::array sources{
        ctex::io::resolution_replay_source(
            eligible, ctex::io::assess_operation_replay(eligible, support, true)),
        ctex::io::resolution_replay_source(
            checkpoint, ctex::io::assess_operation_replay(checkpoint, support, true)),
    };
    const std::vector<std::byte> pixels = replayed_tip();
    const std::array rasters{ResolutionReplayRaster{
        .semantic_id = "pbr.base_color", .udim_tile_number = 0, .pixels = pixels}};

    const bool explicit_fallback = expect_error(
        [&] {
            static_cast<void>(change_texture_set_resolution(
                set, {.width = 4,
                      .height = 4,
                      .policy = ResolutionChangePolicy::replay_eligible,
                      .checkpoint_policy = CheckpointResamplePolicy::refuse,
                      .replay_sources = sources,
                      .replay_rasters = rasters}));
        },
        ResolutionChangeErrorCode::replay_unavailable,
        "mixed replay accepted a checkpoint without an explicit resampling policy");
    const auto unchanged = set.descriptor();
    const auto unchanged_pixel = set.channels().pixels("pbr.base_color").read_pixel(0, 0);
    const auto report =
        change_texture_set_resolution(set, {.width = 4,
                                            .height = 4,
                                            .policy = ResolutionChangePolicy::replay_eligible,
                                            .checkpoint_policy = CheckpointResamplePolicy::bilinear,
                                            .replay_sources = sources,
                                            .replay_rasters = rasters});
    const auto target_tip = set.channels().pixels("pbr.base_color").read_pixel(3, 3);
    const auto old_tip_location = set.channels().pixels("pbr.base_color").read_pixel(0, 0);
    return explicit_fallback && expect(unchanged.width == 2, "failed replay changed dimensions") &&
           expect(unchanged_pixel[2] == std::byte{255}, "failed replay changed source pixels") &&
           expect(report.committed && report.replayed_source_count == 1 &&
                      report.resampled_source_count == 1 && report.procedural_entry_count == 1,
                  "mixed replay report did not classify all content") &&
           expect(set.descriptor().width == 4 && set.descriptor().height == 4,
                  "replay did not publish target dimensions") &&
           expect(target_tip[0] == std::byte{255} && target_tip[1] == std::byte{0} &&
                      old_tip_location[2] == std::byte{0},
                  "eligible stroke was upscaled instead of rerasterized at target resolution");
}

bool resample_is_budgeted_atomic_and_undoable() {
    TextureSet set = texture_set(true);
    static_cast<void>(set.ensure_udim_tiles(std::array<std::uint32_t, 1>{1001}));
    set.udim_channels(1001)
        .pixels("pbr.base_color")
        .write_pixel(1, 1, std::array{std::byte{255}, std::byte{0}, std::byte{0}});
    const auto before_pixel = set.channels().pixels("pbr.base_color").read_pixel(0, 0);
    const bool budgeted = expect_error(
        [&] {
            static_cast<void>(change_texture_set_resolution(
                set, {.width = 4,
                      .height = 4,
                      .policy = ResolutionChangePolicy::resample_all,
                      .checkpoint_policy = CheckpointResamplePolicy::nearest,
                      .maximum_working_bytes = 1}));
        },
        ResolutionChangeErrorCode::over_budget, "over-budget resize was not refused");
    const bool refusal_atomic = set.descriptor().width == 2 && set.descriptor().height == 2;
    const std::array raster_only_source{
        ResolutionReplaySource{.identifier = "unsupported-but-flattened",
                               .disposition = ResolutionReplayDisposition::unsupported_algorithm,
                               .checkpoint_available = false}};
    const auto report =
        change_texture_set_resolution(set, {.width = 4,
                                            .height = 4,
                                            .policy = ResolutionChangePolicy::resample_all,
                                            .checkpoint_policy = CheckpointResamplePolicy::nearest,
                                            .replay_sources = raster_only_source});
    const auto udim_pixel = set.udim_channels(1001).pixels("pbr.base_color").read_pixel(3, 3);
    TextureSet independent_copy = set;
    const auto copy_undone = undo_texture_set_resolution(independent_copy);
    const bool copy_isolated = copy_undone.width == 2 && set.descriptor().width == 4;
    const auto undone = undo_texture_set_resolution(set);
    const auto restored_pixel = set.channels().pixels("pbr.base_color").read_pixel(0, 0);
    const auto redone = redo_texture_set_resolution(set);
    return budgeted && expect(refusal_atomic, "budget refusal changed source state") &&
           expect(report.committed && report.staged_pixel_bytes == 96 &&
                      report.resampled_source_count == 1,
                  "resample report did not include base and UDIM bytes") &&
           expect(udim_pixel[0] == std::byte{255}, "UDIM content was not resampled") &&
           expect(copy_isolated, "copied texture sets shared mutable resolution history") &&
           expect(undone.width == 2 && restored_pixel[2] == before_pixel[2],
                  "resolution undo did not restore dimensions and pixels") &&
           expect(redone.width == 4 && set.descriptor().height == 4,
                  "resolution redo did not restore target state");
}

bool cancellation_and_missing_output_leave_source_intact() {
    TextureSet set = texture_set();
    const auto cancelled = change_texture_set_resolution(
        set, {.width = 8, .height = 8, .policy = ResolutionChangePolicy::cancel});
    const bool missing = expect_error(
        [&] {
            static_cast<void>(change_texture_set_resolution(
                set, {.width = 4,
                      .height = 4,
                      .policy = ResolutionChangePolicy::replay_eligible,
                      .checkpoint_policy = CheckpointResamplePolicy::bilinear}));
        },
        ResolutionChangeErrorCode::missing_replay_output,
        "replay committed without the complete target raster set");
    return expect(!cancelled.committed && set.descriptor().width == 2,
                  "cancel policy changed the texture set") &&
           missing &&
           expect(set.descriptor().height == 2,
                  "missing replay output partially changed the texture set");
}

}  // namespace

int main() {
    return mixed_replay_rasterizes_at_target_resolution() &&
                   resample_is_budgeted_atomic_and_undoable() &&
                   cancellation_and_missing_output_leave_source_intact()
               ? 0
               : 1;
}
