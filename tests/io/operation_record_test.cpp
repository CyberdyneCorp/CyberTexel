#include <array>
#include <cstddef>
#include <ctex/io/operation_record.hpp>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using ctex::io::EditableOperationRecord;
using ctex::io::OperationRecordError;

bool expect(bool condition, std::string_view message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const OperationRecordError&) {
        return true;
    }
    std::cerr << message << '\n';
    return false;
}

EditableOperationRecord fixture() {
    return {
        .identifier = "operations/stroke-42",
        .algorithm_identifier = "cybertexel.paint.brush",
        .algorithm_version = 3,
        .preset_identifier = "brushes/chalk",
        .preset_version = 7,
        .replay_class = ctex::io::OperationReplayClass::resolution_independent,
        .input_document_revision = 41,
        .seed = 0x12345678,
        .mesh_content_identity = "sha256:mesh-v2",
        .coordinate_frame = {1.0, 0.0, 0.0, 4.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0, 6.0, 0.0, 0.0,
                             0.0, 1.0},
        .payload_kind = ctex::io::OperationPayloadKind::resolved_stamps,
        .payload_version = 2,
        .channels = {{.semantic_id = "pbr.base_color",
                      .format = {ctex::image::ChannelType::uint8_unorm, 3},
                      .color_space = ctex::image::ColorSpace::srgb_rec709,
                      .default_value = {0.0, 0.0, 0.0}}},
        .pinned_resources = {{.role = "tip-alpha",
                              .content_identity = "sha256:alpha-original",
                              .bytes = {std::byte{0}, std::byte{127}, std::byte{255}}}},
        .checkpoint_image_identifiers = {"checkpoints/stroke-42/base-color"},
        .payload = {std::byte{4}, std::byte{2}, std::byte{9}},
    };
}

ctex::io::StoredTiledImage checkpoint() {
    ctex::image::TiledImage image(1, 1, {ctex::image::ChannelType::uint8_unorm, 3});
    image.write_pixel(0, 0, std::array{std::byte{9}, std::byte{8}, std::byte{7}});
    return ctex::io::snapshot_tiled_image("checkpoints/stroke-42/base-color", image);
}

bool canonical_record_round_trips() {
    const EditableOperationRecord source = fixture();
    const std::vector<std::byte> first = ctex::io::serialize_operation_record(source);
    const EditableOperationRecord decoded = ctex::io::deserialize_operation_record(first);
    const std::vector<std::byte> second = ctex::io::serialize_operation_record(decoded);
    return expect(decoded == source, "operation record did not round-trip exactly") &&
           expect(first == second, "unchanged operation record was not byte deterministic");
}

bool project_container_keeps_pinned_inputs_and_checkpoint() {
    EditableOperationRecord source = fixture();
    std::vector<std::byte> mutable_shelf_copy = source.pinned_resources.front().bytes;
    ctex::io::ProjectContainer project;
    project.tiled_images.push_back(checkpoint());
    project.assets.push_back(ctex::io::package_operation_record(source));
    mutable_shelf_copy.assign(3, std::byte{99});

    const auto bytes = ctex::io::write_project_container(project);
    const auto opened = ctex::io::read_project_container(bytes);
    const EditableOperationRecord restored =
        ctex::io::unpack_operation_record(opened.container.assets.front());
    return expect(restored == source, "project container changed the operation record") &&
           expect(restored.pinned_resources.front().bytes != mutable_shelf_copy,
                  "operation replay retained mutable shelf bytes instead of pinned content") &&
           expect(opened.container.tiled_images.front() == project.tiled_images.front(),
                  "operation checkpoint did not round-trip with its record");
}

bool malformed_and_over_limit_records_are_refused() {
    EditableOperationRecord invalid = fixture();
    invalid.pinned_resources.front().bytes.clear();
    const bool invalid_resource =
        expect_error([&] { static_cast<void>(ctex::io::serialize_operation_record(invalid)); },
                     "empty pinned input was accepted");

    const std::vector<std::byte> serialized = ctex::io::serialize_operation_record(fixture());
    const bool bounded = expect_error(
        [&] {
            static_cast<void>(ctex::io::deserialize_operation_record(
                serialized, {.maximum_input_bytes = serialized.size() - 1}));
        },
        "operation-record input ceiling was ignored");
    const bool truncated = expect_error(
        [&] {
            static_cast<void>(ctex::io::deserialize_operation_record(
                std::span(serialized).first(serialized.size() - 1)));
        },
        "truncated operation record was accepted");
    return invalid_resource && bounded && truncated;
}

bool replay_eligibility_is_explicit_and_snapshot_safe() {
    EditableOperationRecord source = fixture();
    const std::array supported{
        ctex::io::OperationAlgorithmSupport{
            .identifier = "cybertexel.paint.brush", .minimum_version = 2, .maximum_version = 3},
    };
    const ctex::io::OperationReplayAssessment eligible =
        ctex::io::assess_operation_replay(source, supported, true);

    source.algorithm_version = 4;
    const ctex::io::OperationReplayAssessment unavailable =
        ctex::io::assess_operation_replay(source, supported, false);

    source.algorithm_version = 3;
    source.replay_class = ctex::io::OperationReplayClass::same_resolution;
    const ctex::io::OperationReplayAssessment resample =
        ctex::io::assess_operation_replay(source, supported, true);

    EditableOperationRecord clone = fixture();
    clone.algorithm_identifier = "cybertexel.paint.clone";
    const bool missing_source =
        expect_error([&] { static_cast<void>(ctex::io::serialize_operation_record(clone)); },
                     "clone replay accepted a mutable or missing source snapshot");
    clone.pinned_resources.front().role = "source-snapshot";
    const std::array clone_support{
        ctex::io::OperationAlgorithmSupport{
            .identifier = "cybertexel.paint.clone", .minimum_version = 3, .maximum_version = 3},
    };
    const ctex::io::OperationReplayAssessment pinned =
        ctex::io::assess_operation_replay(clone, clone_support, false);

    return expect(eligible.replay_available &&
                      eligible.disposition ==
                          ctex::io::OperationReplayDisposition::replay_resolution_independent,
                  "resolution-independent record was not replay eligible") &&
           expect(!unavailable.replay_available && unavailable.checkpoint_available &&
                      unavailable.disposition ==
                          ctex::io::OperationReplayDisposition::unsupported_algorithm,
                  "unknown algorithm version did not retain checkpoint fallback") &&
           expect(!resample.replay_available && resample.checkpoint_available &&
                      resample.disposition ==
                          ctex::io::OperationReplayDisposition::resample_checkpoint,
                  "same-resolution replay did not require explicit resampling") &&
           missing_source &&
           expect(pinned.replay_available,
                  "clone with a pinned source snapshot was not replay eligible");
}

}  // namespace

int main() {
    return canonical_record_round_trips() &&
                   project_container_keeps_pinned_inputs_and_checkpoint() &&
                   malformed_and_over_limit_records_are_refused() &&
                   replay_eligibility_is_explicit_and_snapshot_safe()
               ? 0
               : 1;
}
