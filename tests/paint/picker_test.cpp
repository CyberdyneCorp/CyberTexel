#include <array>
#include <ctex/paint/picker.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::paint;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

PaintToolChannelRaster channel(std::string semantic_id, float red) {
    return {.semantic_id = std::move(semantic_id),
            .component_count = 1,
            .pixels = {{red, 0.0F, 0.0F, 1.0F},
                       {red + 0.01F, 0.0F, 0.0F, 1.0F},
                       {red + 0.02F, 0.0F, 0.0F, 1.0F},
                       {red + 0.03F, 0.0F, 0.0F, 1.0F}}};
}

std::vector<PaintToolChannelRaster> pbr_channels() {
    return {channel("pbr.base_color", 0.1F), channel("pbr.roughness", 0.2F),
            channel("pbr.metallic", 0.3F),   channel("pbr.normal", 0.4F),
            channel("pbr.height", 0.5F),     channel("pbr.occlusion", 0.6F),
            channel("pbr.emission", 0.7F),   channel("pbr.subsurface", 0.8F)};
}

pick::HitRecord hit() {
    return {.position = {},
            .interpolated_normal = {0.0F, 0.0F, 1.0F},
            .geometric_normal = {0.0F, 0.0F, 1.0F},
            .uv = {1.25F, 0.75F},
            .texture_set_id = "set:body",
            .udim_tile = {.u = 1, .v = 0, .number = 1002},
            .triangle_index = 0,
            .barycentric = {1.0F, 0.0F, 0.0F},
            .material_id = 7,
            .distance = 1.0F};
}

bool picker_reads_every_enabled_channel_and_optional_material() {
    const std::vector channels = pbr_channels();
    const std::array<std::string_view, 4> provenance{"material:paint", "material:edge",
                                                     "material:dirt", "material:metal"};
    const std::array views{
        PickerTextureView{.texture_set_id = "set:body",
                          .tile_origin = {0.0, 0.0},
                          .width = 2,
                          .height = 2,
                          .enabled_channels = channels,
                          .material_provenance = std::nullopt},
        PickerTextureView{.texture_set_id = "set:body",
                          .tile_origin = {1.0, 0.0},
                          .width = 2,
                          .height = 2,
                          .enabled_channels = channels,
                          .material_provenance = MaterialProvenanceView{provenance}}};
    const PickerResult result = pick_enabled_channels(hit(), views);
    const std::array expected_ids{"pbr.base_color", "pbr.roughness", "pbr.metallic",
                                  "pbr.normal",     "pbr.height",    "pbr.occlusion",
                                  "pbr.emission",   "pbr.subsurface"};
    bool channels_match = result.channels.size() == expected_ids.size();
    for (std::size_t index = 0; index < result.channels.size() && channels_match; ++index) {
        channels_match = result.channels[index].semantic_id == expected_ids[index] &&
                         result.channels[index].value.r == 0.1F * static_cast<float>(index + 1);
    }

    return expect(result.texture_set_id == "set:body" && result.tile_origin == Vec2d{1.0, 0.0} &&
                      result.texel == 0,
                  "picker did not resolve the hit's texture set and UDIM tile") &&
           expect(channels_match,
                  "picker did not return every enabled channel in declared order") &&
           expect(result.material_identity == "material:paint",
                  "picker did not return requested material provenance");
}

bool material_selection_is_absent_when_provenance_is_not_requested() {
    const std::vector channels = pbr_channels();
    const std::array views{PickerTextureView{.texture_set_id = "set:body",
                                             .tile_origin = {1.0, 0.0},
                                             .width = 2,
                                             .height = 2,
                                             .enabled_channels = channels,
                                             .material_provenance = std::nullopt}};
    const PickerResult result = pick_enabled_channels(hit(), views);
    return expect(!result.material_identity.has_value(),
                  "picker selected a material without supplied provenance");
}

bool missing_overlapping_and_invalid_views_are_refused() {
    const std::vector channels = pbr_channels();
    const PickerTextureView matching{.texture_set_id = "set:body",
                                     .tile_origin = {1.0, 0.0},
                                     .width = 2,
                                     .height = 2,
                                     .enabled_channels = channels,
                                     .material_provenance = std::nullopt};
    bool missing_refused = false;
    try {
        const std::array views{PickerTextureView{.texture_set_id = "set:other",
                                                 .tile_origin = {1.0, 0.0},
                                                 .width = 2,
                                                 .height = 2,
                                                 .enabled_channels = channels,
                                                 .material_provenance = std::nullopt}};
        static_cast<void>(pick_enabled_channels(hit(), views));
    } catch (const std::invalid_argument&) {
        missing_refused = true;
    }
    bool overlap_refused = false;
    try {
        const std::array views{matching, matching};
        static_cast<void>(pick_enabled_channels(hit(), views));
    } catch (const std::invalid_argument&) {
        overlap_refused = true;
    }
    bool channels_refused = false;
    try {
        const std::array invalid_channels{PaintToolChannelRaster{
            .semantic_id = "pbr.base_color", .component_count = 1, .pixels = {}}};
        const std::array views{PickerTextureView{.texture_set_id = "set:body",
                                                 .tile_origin = {1.0, 0.0},
                                                 .width = 2,
                                                 .height = 2,
                                                 .enabled_channels = invalid_channels,
                                                 .material_provenance = std::nullopt}};
        static_cast<void>(pick_enabled_channels(hit(), views));
    } catch (const std::invalid_argument&) {
        channels_refused = true;
    }
    bool unrepresentable_tile_refused = false;
    try {
        const std::array views{PickerTextureView{.texture_set_id = "set:body",
                                                 .tile_origin = {1.0e308, 0.0},
                                                 .width = 2,
                                                 .height = 2,
                                                 .enabled_channels = channels,
                                                 .material_provenance = std::nullopt}};
        static_cast<void>(pick_enabled_channels(hit(), views));
    } catch (const std::invalid_argument&) {
        unrepresentable_tile_refused = true;
    }
    return expect(
        missing_refused && overlap_refused && channels_refused && unrepresentable_tile_refused,
        "missing, overlapping or invalid picker views were not refused");
}

}  // namespace

int main() {
    return picker_reads_every_enabled_channel_and_optional_material() &&
                   material_selection_is_absent_when_provenance_is_not_requested() &&
                   missing_overlapping_and_invalid_views_are_refused()
               ? 0
               : 1;
}
